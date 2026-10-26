#pragma once

#include <string>
#include <memory>
#include <vector>

class QSqlDatabase;

struct bank_account_record
{
    std::string account;
    std::string pin;
    bool exists = false;
    bool pin_matches = false;
    bool is_admin = false;
    unsigned balance = 0;
};

// Small SQLite repository used by the bank actor. The actor is single-threaded,
// so the repository deliberately keeps its API synchronous and simple.
class bank_database
{
public:
    explicit bank_database(std::string path = std::string());
    ~bank_database();

    bank_database(bank_database const&) = delete;
    bank_database& operator=(bank_database const&) = delete;

    bank_account_record authenticate(std::string const& account,
                                     std::string const& pin) const;
    bool is_admin_account(std::string const& account) const;
    unsigned balance(std::string const& account) const;
    bool withdraw(std::string const& account, unsigned amount);
    bool create_account(std::string const& account, std::string const& pin, unsigned balance);
    bool set_balance(std::string const& account, unsigned balance);
    bool set_pin(std::string const& account, std::string const& pin);
    bool delete_account(std::string const& account);
    std::vector<bank_account_record> list_accounts() const;

private:
    std::string m_connectionName;
    std::unique_ptr<QSqlDatabase> m_db;
    void initialize();
};
