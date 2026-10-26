#include "app/main_window.hpp"

#include <QCloseEvent>
#include <QDateTime>
#include <QFontDatabase>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>
#include <QKeyEvent>
#include <QEvent>
#include <functional>

#include "core/messages/atm_messages.hpp"
#include "core/messages/bank_messages.hpp"
#include "app/admin_window.hpp"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_bridge(this),
      m_uiActor([this](std::string const& text) {
          m_bridge.emitLine(QString::fromStdString(text));
      }),
      m_bank(),
      m_atm(m_bank.get_sender(), m_uiActor.get_sender()),
      m_atmSender(m_atm.get_sender())
{
    setWindowTitle(QStringLiteral("ATM 模拟器"));
    resize(460, 760);
    setMinimumSize(420, 700);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(12);
    m_screen = new QLabel(QStringLiteral("请输入银行卡号"), central);
    m_screen->setObjectName(QStringLiteral("screen"));
    m_screen->setAlignment(Qt::AlignCenter);
    m_screen->setMinimumHeight(132);
    m_screen->setWordWrap(true);
    m_screen->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_screen);

    auto* cardRow = new QHBoxLayout;
    cardRow->setSpacing(12);
    auto* cardLabel = new QLabel(QStringLiteral("银行卡号"), central);
    cardLabel->setMinimumWidth(70);
    cardRow->addWidget(cardLabel);
    m_cardNumber = new QLineEdit(central);
    m_cardNumber->setPlaceholderText(QStringLiteral("请输入银行卡号"));
    m_cardNumber->clear();
    m_cardNumber->setMaxLength(32);
    m_cardNumber->setReadOnly(true);
    m_cardNumber->setToolTip(QStringLiteral("点击“输入银行卡”后解锁，回车提交"));
    connect(m_cardNumber, &QLineEdit::returnPressed, this, &MainWindow::submitCard);
    cardRow->addWidget(m_cardNumber);
    layout->addLayout(cardRow);

    auto* pinRow = new QHBoxLayout;
    auto* pinLabel = new QLabel(QStringLiteral("密码"), central);
    pinLabel->setMinimumWidth(70);
    pinRow->addWidget(pinLabel);
    m_pinInput = new QLineEdit(central);
    m_pinInput->setEchoMode(QLineEdit::Password);
    m_pinInput->setMaxLength(4);
    m_pinInput->setReadOnly(true);
    m_pinInput->setPlaceholderText(QStringLiteral("请输入 4 位密码"));
    m_pinInput->installEventFilter(this);
    pinRow->addWidget(m_pinInput);
    layout->addLayout(pinRow);

    auto* keypad = new QGridLayout;
    keypad->setSpacing(8);
    const QString keys[4][3] = {{QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")},
                                {QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6")},
                                {QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9")},
                                {QStringLiteral("清除"), QStringLiteral("0"), QStringLiteral("取消")}};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            auto* button = new QPushButton(keys[row][column], central);
            button->setMinimumHeight(44);
            if (row == 3 && column == 2) button->setObjectName(QStringLiteral("dangerButton"));
            m_buttons.push_back(button);
            keypad->addWidget(button, row, column);
            if (row < 3 || column == 1) {
                const char digit = (row < 3) ? static_cast<char>('1' + row * 3 + column) : '0';
                connect(button, &QPushButton::clicked, this, [this, digit] { sendDigit(digit); });
            } else if (column == 0) {
                connect(button, &QPushButton::clicked, this, [this] {
                    if (m_withdrawAmount->isEnabled()) {
                        QLineEdit* editor = m_withdrawAmount->findChild<QLineEdit*>();
                        QString digits = editor ? editor->text() : QString();
                        if (!digits.isEmpty()) digits.chop(1);
                        m_withdrawAmount->setValue(digits.isEmpty() ? 0 : digits.toInt());
                    } else if (!m_pinInput->isReadOnly()) {
                        m_pinInput->backspace();
                        m_atmSender.send(clear_last_pressed());
                    }
                });
            } else {
                connect(button, &QPushButton::clicked, this, [this] {
                    if (!m_withdrawalPending) m_atmSender.send(cancel_pressed());
                });
            }
        }
    }
    layout->addLayout(keypad);

    auto* actions = new QHBoxLayout;
    const auto addAction = [this, actions, central](QString const& text, std::function<void()> fn) {
        auto* button = new QPushButton(text, central);
        if (text == QStringLiteral("取款")) button->setObjectName(QStringLiteral("primaryButton"));
        button->setMinimumHeight(40);
        m_buttons.push_back(button);
        actions->addWidget(button);
        connect(button, &QPushButton::clicked, this, [fn] { fn(); });
    };
    m_cardButton = new QPushButton(QStringLiteral("输入银行卡"), central);
    m_cardButton->setMinimumHeight(40);
    m_buttons.push_back(m_cardButton);
    actions->addWidget(m_cardButton);
    connect(m_cardButton, &QPushButton::clicked, this, [this] {
        if (m_cardNumber->isReadOnly()) {
            m_cardNumber->setReadOnly(false);
            m_cardNumber->selectAll();
            m_cardNumber->setFocus();
            m_cardButton->setText(QStringLiteral("确认银行卡"));
        } else {
            submitCard();
        }
    });
    auto* amountRow = new QHBoxLayout;
    auto* amountLabel = new QLabel(QStringLiteral("取款金额"), central);
    amountLabel->setMinimumWidth(70);
    amountRow->addWidget(amountLabel);
    m_withdrawAmount = new QSpinBox(central);
    m_withdrawAmount->setRange(0, 1000000);
    m_withdrawAmount->setSingleStep(50);
    m_withdrawAmount->setValue(0);
    m_withdrawAmount->setEnabled(false);
    m_withdrawAmount->installEventFilter(this);
    amountRow->addWidget(m_withdrawAmount);
    layout->addLayout(amountRow);
    addAction(QStringLiteral("取款"), [this] {
        if (m_withdrawAmount->isEnabled()) {
            if (m_withdrawAmount->value() <= 0) {
                m_screen->setText(QStringLiteral("请输入大于 0 的取款金额"));
                return;
            }
            m_atmSender.send(withdraw_pressed(static_cast<unsigned>(m_withdrawAmount->value())));
            m_withdrawAmount->setEnabled(false);
            m_withdrawalPending = true;
            setButtonsEnabled(false);
        } else {
            m_atmSender.send(withdraw_pressed(0));
        }
    });
    addAction(QStringLiteral("查余额"), [this] { m_atmSender.send(balance_pressed()); });
    addAction(QStringLiteral("退卡"), [this] { m_atmSender.send(eject_pressed()); });
    layout->addLayout(actions);

    m_log = new QPlainTextEdit(central);
    m_log->setReadOnly(true);
    m_log->setFixedHeight(140);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_log);
    m_status = new QLabel(QStringLiteral("已连接"), central);
    m_status->setObjectName(QStringLiteral("status"));
    layout->addWidget(m_status);
    setCentralWidget(central);

    connect(&m_bridge, &QtDisplayBridge::lineReady,
            this, &MainWindow::appendDisplay,
            Qt::QueuedConnection);

    m_bankThread = std::thread(&bank_machine::run, &m_bank);
    m_uiThread = std::thread(&interface_machine::run, &m_uiActor);
    m_atmThread = std::thread(&atm::run, &m_atm);
}

void MainWindow::submitCard()
{
    const QString account = m_cardNumber->text().trimmed();
    if (account.isEmpty()) {
        m_screen->setText(QStringLiteral("请输入银行卡号"));
        m_cardNumber->setReadOnly(false);
        m_cardButton->setText(QStringLiteral("确认银行卡"));
        m_cardNumber->setFocus();
        return;
    }
    m_atmSender.send(card_inserted(account.toStdString()));
    m_cardNumber->setReadOnly(true);
    m_cardButton->setText(QStringLiteral("银行卡已锁定"));
    m_pinInput->clear();
    m_pinInput->setReadOnly(false);
    m_pinInput->setFocus();
}

void MainWindow::sendDigit(char digit)
{
    if (m_withdrawAmount && m_withdrawAmount->isEnabled()) {
        appendWithdrawDigit(digit);
        return;
    }
    if (!m_pinInput || m_pinInput->isReadOnly() || m_pinInput->text().size() >= 4) return;
    m_pinInput->insert(QChar(digit));
    m_atmSender.send(digit_pressed(digit));
}

void MainWindow::appendWithdrawDigit(char digit)
{
    if (!m_withdrawAmount || !m_withdrawAmount->isEnabled()) return;
    QLineEdit* editor = m_withdrawAmount->findChild<QLineEdit*>();
    QString digits = editor ? editor->text() : QString();
    if (digits == QStringLiteral("0")) digits.clear();
    if (digits.size() >= 7) return;
    digits += QChar(digit);
    bool ok = false;
    const int amount = digits.toInt(&ok);
    if (ok && amount <= m_withdrawAmount->maximum()) {
        m_withdrawAmount->setValue(amount);
    }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_pinInput || watched == m_cardNumber || watched == m_withdrawAmount) && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {
            if (watched == m_cardNumber && !m_cardNumber->isReadOnly()) submitCard();
            else if (watched == m_pinInput && m_pinInput->text().size() == 4) {
                m_atmSender.send(pin_confirm_pressed());
                m_pinInput->setReadOnly(true);
            } else if (watched == m_withdrawAmount && m_withdrawAmount->isEnabled()) {
                if (m_withdrawAmount->value() <= 0) {
                    m_screen->setText(QStringLiteral("请输入大于 0 的取款金额"));
                    return true;
                }
                m_atmSender.send(withdraw_pressed(static_cast<unsigned>(m_withdrawAmount->value())));
                m_withdrawAmount->setEnabled(false);
                m_withdrawalPending = true;
                setButtonsEnabled(false);
            }
            return true;
        }
        if (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_9) {
            sendDigit(static_cast<char>('0' + keyEvent->key() - Qt::Key_0));
            return true;
        }
        if (watched == m_pinInput && keyEvent->key() == Qt::Key_Backspace) {
            if (!m_pinInput->text().isEmpty()) m_pinInput->backspace();
            m_atmSender.send(clear_last_pressed());
            return true;
        }
        if (keyEvent->key() == Qt::Key_Q && !m_withdrawalPending) {
            m_atmSender.send(eject_pressed());
            return true;
        }
        if ((keyEvent->key() == Qt::Key_Escape || keyEvent->key() == Qt::Key_C) && !m_withdrawalPending) {
            m_atmSender.send(cancel_pressed());
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resetPinInput()
{
    if (m_pinInput) m_pinInput->clear();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_cardNumber && !m_cardNumber->isReadOnly()) submitCard();
        else if (m_pinInput && !m_pinInput->isReadOnly() && m_pinInput->text().size() == 4) {
            m_atmSender.send(pin_confirm_pressed());
            m_pinInput->setReadOnly(true);
        }
        event->accept();
        return;
    }
    if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
        sendDigit(static_cast<char>('0' + event->key() - Qt::Key_0));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Backspace && m_pinInput && !m_pinInput->isReadOnly()) {
        m_pinInput->backspace();
        m_atmSender.send(clear_last_pressed());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && !m_withdrawalPending) {
        m_atmSender.send(cancel_pressed());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Q && !m_withdrawalPending) {
        m_atmSender.send(eject_pressed());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_I) {
        if (m_cardNumber->isReadOnly() && m_pinInput->isReadOnly()) {
            m_cardNumber->setReadOnly(false);
            m_cardNumber->selectAll();
            m_cardNumber->setFocus();
            m_cardButton->setText(QStringLiteral("确认银行卡"));
        } else if (!m_cardNumber->isReadOnly()) {
            submitCard();
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_B) {
        m_atmSender.send(balance_pressed());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_W) {
        if (m_withdrawAmount->isEnabled()) {
            if (m_withdrawAmount->value() <= 0) {
                m_screen->setText(QStringLiteral("请输入大于 0 的取款金额"));
                event->accept();
                return;
            }
            m_atmSender.send(withdraw_pressed(static_cast<unsigned>(m_withdrawAmount->value())));
            m_withdrawAmount->setEnabled(false);
            m_withdrawalPending = true;
            setButtonsEnabled(false);
        } else {
            m_atmSender.send(withdraw_pressed(0));
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_C) {
        if (!m_withdrawalPending) m_atmSender.send(cancel_pressed());
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

MainWindow::~MainWindow()
{
    if (m_joiner.joinable()) {
        m_joiner.join();
    }
}

void MainWindow::appendDisplay(QString const& line)
{
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")), line));
    m_screen->setText(line);
    if (line.contains(QStringLiteral("管理员已登录"))) {
        resetToInitialState();
        if (!m_adminWindow) {
            m_adminWindow = new AdminWindow(this);
        }
        m_adminWindow->show();
        m_adminWindow->raise();
        m_adminWindow->activateWindow();
    }
    if (line.contains(QStringLiteral("请输入密码"))) {
        m_pinInput->clear();
        m_pinInput->setReadOnly(false);
        m_pinInput->setFocus();
    }
    if (line.contains(QStringLiteral("正在取款")) || line.contains(QStringLiteral("请重新输入取款金额"))) {
        m_withdrawAmount->setEnabled(true);
        m_withdrawAmount->setValue(0);
        if (QLineEdit* editor = m_withdrawAmount->findChild<QLineEdit*>()) editor->clear();
        m_withdrawAmount->setFocus();
        m_withdrawAmount->selectAll();
        m_withdrawalPending = false;
    }
    if (line.contains(QStringLiteral("正在出钞")) ||
        line.contains(QStringLiteral("取款已取消"))) {
        m_withdrawalPending = false;
        setButtonsEnabled(true);
    }
    if (line.contains(QStringLiteral("正在出钞"))) {
        m_withdrawAmount->setEnabled(false);
    }
    if (line.contains(QStringLiteral("余额不足"))) {
        m_withdrawAmount->setValue(0);
        if (QLineEdit* editor = m_withdrawAmount->findChild<QLineEdit*>()) editor->clear();
        m_withdrawAmount->setEnabled(false);
    }
    if (line.contains(QStringLiteral("请输入取款金额"))) {
        m_withdrawAmount->setEnabled(true);
        m_withdrawAmount->setFocus();
    }
    if (line.contains(QStringLiteral("取款已取消"))) {
        m_withdrawAmount->setEnabled(false);
    }
    if (line.contains(QStringLiteral("退卡"))) {
        m_withdrawalPending = false;
        setButtonsEnabled(true);
        m_pinInput->clear();
        m_pinInput->setReadOnly(true);
        m_cardNumber->setReadOnly(true);
        m_cardButton->setText(QStringLiteral("输入银行卡"));
    }
}

void MainWindow::resetToInitialState()
{
    m_cardNumber->clear();
    m_cardNumber->setReadOnly(true);
    m_cardButton->setText(QStringLiteral("输入银行卡"));
    m_pinInput->clear();
    m_pinInput->setReadOnly(true);
    m_withdrawAmount->setValue(0);
    m_withdrawAmount->setEnabled(false);
}

void MainWindow::setButtonsEnabled(bool enabled)
{
    for (QPushButton* button : m_buttons) {
        button->setEnabled(enabled);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_readyToClose) {
        event->accept();
        return;
    }
    if (m_withdrawalPending) {
        m_status->setText(QStringLiteral("请等待取款处理完成后关闭"));
        event->ignore();
        return;
    }
    if (m_closing) {
        event->ignore();
        return;
    }
    m_closing = true;
    event->ignore();
    setEnabled(false);
    setButtonsEnabled(false);
    setWindowTitle(QStringLiteral("正在关闭…"));
    m_status->setText(QStringLiteral("正在关闭…"));
    m_atm.done();
    m_bank.done();
    m_uiActor.done();
    m_joiner = std::thread([this] {
        if (m_atmThread.joinable()) m_atmThread.join();
        if (m_uiThread.joinable()) m_uiThread.join();
        if (m_bankThread.joinable()) m_bankThread.join();
        QMetaObject::invokeMethod(this, [this] {
            m_readyToClose = true;
            close();
        }, Qt::QueuedConnection);
    });
}
