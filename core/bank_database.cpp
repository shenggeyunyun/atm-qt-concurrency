#include "core/bank_database.hpp"

#include <cstdlib>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
std::string database_path(std::string path)
{
    if (!path.empty()) return path;
    if (const char* configured = std::getenv("ATM_DB_PATH")) {
        if (*configured) return configured;
    }
    return "atm.sqlite3";
}

void exec_sql(QSqlDatabase const& db, char const* sql)
{
    QSqlQuery query(db);
    if (!query.exec(QString::fromUtf8(sql))) throw std::runtime_error(query.lastError().text().toStdString());
}
}

bank_database::bank_database(std::string path)
{
    m_connectionName = "atm_bank_" + std::to_string(reinterpret_cast<std::uintptr_t>(this));
    m_db.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", QString::fromStdString(m_connectionName))));
    m_db->setDatabaseName(QString::fromStdString(database_path(std::move(path))));
    if (!m_db->open()) throw std::runtime_error(m_db->lastError().text().toStdString());
    exec_sql(*m_db, "PRAGMA foreign_keys = ON;");
    initialize();
}

bank_database::~bank_database()
{
    if (m_db) {
        m_db->close();
        m_db.reset();
        QSqlDatabase::removeDatabase(QString::fromStdString(m_connectionName));
    }
}

void bank_database::initialize()
{
    // Qt's SQLite driver accepts one statement per QSqlQuery::exec call.
    exec_sql(*m_db,
        "CREATE TABLE IF NOT EXISTS accounts ("
        "account TEXT PRIMARY KEY, pin TEXT NOT NULL, balance INTEGER NOT NULL DEFAULT 0, is_admin INTEGER NOT NULL DEFAULT 0)");
    exec_sql(*m_db,
        "CREATE TABLE IF NOT EXISTS admins (username TEXT PRIMARY KEY, pin TEXT NOT NULL)");
    exec_sql(*m_db,
        "INSERT OR IGNORE INTO accounts(account,pin,balance,is_admin) VALUES('acc1234','1937',199,0)");
    exec_sql(*m_db,
        "INSERT OR IGNORE INTO accounts(account,pin,balance,is_admin) VALUES('admin','2468',0,1)");
    exec_sql(*m_db,
        "INSERT OR IGNORE INTO admins(username,pin) VALUES('admin','2468')");
}

bank_account_record bank_database::authenticate(std::string const& account,
                                                 std::string const& pin) const
{
    bank_account_record result;
    QSqlQuery statement(*m_db);
    statement.prepare(QStringLiteral("SELECT pin,balance,is_admin FROM accounts WHERE account=:account"));
    statement.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    if (statement.exec() && statement.next()) {
        result.exists = true;
        result.pin_matches = pin == statement.value(0).toString().toStdString();
        result.balance = statement.value(1).toUInt();
        result.is_admin = statement.value(2).toInt() != 0;
    }
    return result;
}

bool bank_database::is_admin_account(std::string const& account) const
{
    QSqlQuery statement(*m_db);
    statement.prepare(QStringLiteral("SELECT 1 FROM accounts WHERE account=:account AND is_admin=1"));
    statement.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    return statement.exec() && statement.next();
}

unsigned bank_database::balance(std::string const& account) const
{
    QSqlQuery statement(*m_db);
    statement.prepare(QStringLiteral("SELECT balance FROM accounts WHERE account=:account"));
    statement.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    return statement.exec() && statement.next() ? statement.value(0).toUInt() : 0;
}

bool bank_database::withdraw(std::string const& account, unsigned amount)
{
    if (amount == 0 || amount > static_cast<unsigned>(std::numeric_limits<int>::max())) return false;
    QSqlQuery statement(*m_db);
    statement.prepare(QStringLiteral("UPDATE accounts SET balance=balance-:amount WHERE account=:account AND is_admin=0 AND balance>=:amount"));
    statement.bindValue(QStringLiteral(":amount"), static_cast<int>(amount));
    statement.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    return statement.exec() && statement.numRowsAffected() == 1;
}

bool bank_database::create_account(std::string const& account, std::string const& pin, unsigned initial_balance)
{
    if (account.empty() || pin.size() != 4 || initial_balance > static_cast<unsigned>(std::numeric_limits<int>::max())) return false;
    for (char digit : pin) if (digit < '0' || digit > '9') return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("INSERT INTO accounts(account,pin,balance,is_admin) VALUES(:account,:pin,:balance,0)"));
    query.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    query.bindValue(QStringLiteral(":pin"), QString::fromStdString(pin));
    query.bindValue(QStringLiteral(":balance"), static_cast<int>(initial_balance));
    return query.exec() && query.numRowsAffected() == 1;
}

bool bank_database::set_balance(std::string const& account, unsigned value)
{
    if (value > static_cast<unsigned>(std::numeric_limits<int>::max())) return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("UPDATE accounts SET balance=:balance WHERE account=:account AND is_admin=0"));
    query.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    query.bindValue(QStringLiteral(":balance"), static_cast<int>(value));
    return query.exec() && query.numRowsAffected() == 1;
}

bool bank_database::set_pin(std::string const& account, std::string const& pin)
{
    if (account.empty() || pin.size() != 4) return false;
    for (char digit : pin) if (digit < '0' || digit > '9') return false;
    if (!m_db->transaction()) return false;
    QSqlQuery accounts_query(*m_db);
    accounts_query.prepare(QStringLiteral("UPDATE accounts SET pin=:pin WHERE account=:account"));
    accounts_query.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    accounts_query.bindValue(QStringLiteral(":pin"), QString::fromStdString(pin));
    if (!accounts_query.exec() || accounts_query.numRowsAffected() != 1) {
        m_db->rollback();
        return false;
    }
    QSqlQuery admins_query(*m_db);
    admins_query.prepare(QStringLiteral("UPDATE admins SET pin=:pin WHERE username=:account"));
    admins_query.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    admins_query.bindValue(QStringLiteral(":pin"), QString::fromStdString(pin));
    if (!admins_query.exec() || !m_db->commit()) {
        m_db->rollback();
        return false;
    }
    return true;
}

bool bank_database::delete_account(std::string const& account)
{
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("DELETE FROM accounts WHERE account=:account AND is_admin=0"));
    query.bindValue(QStringLiteral(":account"), QString::fromStdString(account));
    return query.exec() && query.numRowsAffected() == 1;
}

std::vector<bank_account_record> bank_database::list_accounts() const
{
    std::vector<bank_account_record> accounts;
    QSqlQuery query(*m_db);
    if (!query.exec(QStringLiteral("SELECT account,pin,balance,is_admin FROM accounts ORDER BY account"))) return accounts;
    while (query.next()) {
        bank_account_record record;
        record.account = query.value(0).toString().toStdString();
        record.pin = query.value(1).toString().toStdString();
        record.balance = query.value(2).toUInt();
        record.is_admin = query.value(3).toInt() != 0;
        record.exists = true;
        record.pin_matches = false;
        accounts.push_back(record);
    }
    return accounts;
}
