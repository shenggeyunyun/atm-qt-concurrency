#include "app/admin_window.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include <QFont>
#include <QWidget>

AdminWindow::AdminWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("ATM 后台管理"));
    resize(700, 500);
    setMinimumSize(620, 420);
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(12);
    auto* heading = new QLabel(QStringLiteral("管理员后台 · 账户管理"), central);
    QFont headingFont = heading->font();
    headingFont.setPointSize(14);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    layout->addWidget(heading);
    auto* subtitle = new QLabel(QStringLiteral("仅管理员登录后可用。双击 PIN 或余额单元格即可修改。"), central);
    subtitle->setObjectName(QStringLiteral("eyebrow"));
    layout->addWidget(subtitle);
    m_accounts = new QTableWidget(central);
    m_accounts->setColumnCount(4);
    m_accounts->setHorizontalHeaderLabels({QStringLiteral("账号"), QStringLiteral("PIN"), QStringLiteral("余额"), QStringLiteral("类型")});
    m_accounts->horizontalHeader()->setStretchLastSection(true);
    m_accounts->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_accounts->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_accounts->setSelectionMode(QAbstractItemView::SingleSelection);
    m_accounts->setAlternatingRowColors(true);
    m_accounts->verticalHeader()->setVisible(false);
    m_accounts->verticalHeader()->setDefaultSectionSize(34);
    m_accounts->horizontalHeader()->setMinimumSectionSize(90);
    m_accounts->setShowGrid(false);
    layout->addWidget(m_accounts);

    auto* form = new QFormLayout;
    m_account = new QLineEdit(central);
    m_account->setPlaceholderText(QStringLiteral("普通账户账号"));
    m_pin = new QLineEdit(central);
    m_pin->setPlaceholderText(QStringLiteral("4 位数字 PIN"));
    m_pin->setEchoMode(QLineEdit::Password);
    m_pin->setMaxLength(4);
    m_pin->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{0,4}")), m_pin));
    m_balance = new QSpinBox(central);
    m_balance->setRange(0, 100000000);
    form->addRow(QStringLiteral("账号"), m_account);
    form->addRow(QStringLiteral("密码"), m_pin);
    form->addRow(QStringLiteral("余额"), m_balance);
    layout->addLayout(form);

    auto* create = new QPushButton(QStringLiteral("新增普通账户"), central);
    m_delete = new QPushButton(QStringLiteral("删除选中普通账户"), central);
    auto* clear = new QPushButton(QStringLiteral("清空输入"), central);
    auto* refresh = new QPushButton(QStringLiteral("刷新账户列表"), central);
    auto* actions = new QHBoxLayout;
    actions->setSpacing(8);
    create->setObjectName(QStringLiteral("primaryButton"));
    m_delete->setObjectName(QStringLiteral("dangerButton"));
    for (QPushButton* button : {create, m_delete, clear, refresh}) {
        button->setMinimumHeight(38);
        actions->addWidget(button);
    }
    layout->addLayout(actions);
    m_status = new QLabel(QStringLiteral("等待管理员操作"), central);
    m_status->setObjectName(QStringLiteral("status"));
    layout->addWidget(m_status);
    setCentralWidget(central);
    refreshAccounts();

    connect(create, &QPushButton::clicked, this, [this] {
        if (m_account->text().trimmed().isEmpty() || m_pin->text().size() != 4) {
            m_status->setText(QStringLiteral("请输入账号和 4 位数字密码"));
            return;
        }
        const bool created = m_database.create_account(m_account->text().trimmed().toStdString(),
                                                       m_pin->text().toStdString(), static_cast<unsigned>(m_balance->value()));
        m_status->setText(created ? QStringLiteral("账户创建成功") : QStringLiteral("账户创建失败，账号可能已存在"));
        if (created) refreshAccounts();
    });
    connect(m_accounts, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (item->column() == 1) updatePin(item);
        else if (item->column() == 2) updateBalance(item);
    });
    connect(m_delete, &QPushButton::clicked, this, [this] {
        const QList<QTableWidgetItem*> selected = m_accounts->selectedItems();
        if (selected.isEmpty()) { m_status->setText(QStringLiteral("请先选择账户")); return; }
        const int row = selected.first()->row();
        const QString account = m_accounts->item(row, 0)->text();
        const bool deleted = m_database.delete_account(account.toStdString());
        m_status->setText(deleted ? QStringLiteral("账户已删除") : QStringLiteral("删除失败，账户可能已不存在"));
        if (deleted) refreshAccounts();
    });
    connect(clear, &QPushButton::clicked, this, [this] {
        m_account->clear();
        m_pin->clear();
        m_balance->setValue(0);
        m_status->setText(QStringLiteral("已清空输入"));
    });
    connect(refresh, &QPushButton::clicked, this, [this] { refreshAccounts(); });
}

void AdminWindow::refreshAccounts()
{
    m_accounts->blockSignals(true);
    const std::vector<bank_account_record> records = m_database.list_accounts();
    m_accounts->setRowCount(static_cast<int>(records.size()));
    for (int row = 0; row < static_cast<int>(records.size()); ++row) {
        bank_account_record const& record = records[static_cast<std::size_t>(row)];
        m_accounts->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(record.account)));
        m_accounts->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(record.pin)));
        m_accounts->setItem(row, 2, new QTableWidgetItem(QString::number(record.balance)));
        m_accounts->setItem(row, 3, new QTableWidgetItem(record.is_admin ? QStringLiteral("管理员") : QStringLiteral("普通账户")));
        m_accounts->item(row, 0)->setFlags(m_accounts->item(row, 0)->flags() & ~Qt::ItemIsEditable);
        m_accounts->item(row, 3)->setFlags(m_accounts->item(row, 3)->flags() & ~Qt::ItemIsEditable);
    }
    m_accounts->blockSignals(false);
}

void AdminWindow::updateBalance(QTableWidgetItem* item)
{
    if (!item || item->column() != 2) return;
    bool ok = false;
    const unsigned value = item->text().toUInt(&ok);
    if (!ok) { refreshAccounts(); return; }
    const QString account = m_accounts->item(item->row(), 0)->text();
    const bool updated = m_database.set_balance(account.toStdString(), value);
    m_status->setText(updated ? QStringLiteral("余额修改成功") : QStringLiteral("余额修改失败，账户可能已不存在"));
    refreshAccounts();
}

void AdminWindow::updatePin(QTableWidgetItem* item)
{
    if (!item || item->column() != 1) return;
    const QString pin = item->text().trimmed();
    const QString account = m_accounts->item(item->row(), 0)->text();
    const bool updated = m_database.set_pin(account.toStdString(), pin.toStdString());
    m_status->setText(updated ? QStringLiteral("PIN 修改成功") : QStringLiteral("PIN 修改失败：必须是 4 位数字，且账号必须存在"));
    refreshAccounts();
}
