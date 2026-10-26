#pragma once
#include <QMainWindow>
#include <QString>
#include <thread>
#include <vector>

#include "qt_display_bridge.hpp"
#include "actors/atm_actor.hpp"
#include "actors/bank_actor.hpp"
#include "actors/ui_actor.hpp"
#include "messaging/sender.hpp"

class QCloseEvent;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QKeyEvent;
class QEvent;
class AdminWindow;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public:
    void appendDisplay(QString const& line);

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setButtonsEnabled(bool enabled);
    void submitCard();
    void sendDigit(char digit);
    void appendWithdrawDigit(char digit);
    void resetPinInput();
    void resetToInitialState();

    QtDisplayBridge m_bridge;
    interface_machine m_uiActor;
    bank_machine m_bank;
    atm m_atm;
    std::thread m_bankThread;
    std::thread m_uiThread;
    std::thread m_atmThread;
    messaging::sender m_atmSender;
    QLabel* m_screen = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QLabel* m_status = nullptr;
    QLineEdit* m_cardNumber = nullptr;
    QLineEdit* m_pinInput = nullptr;
    QSpinBox* m_withdrawAmount = nullptr;
    QPushButton* m_cardButton = nullptr;
    QLineEdit* m_adminAccount = nullptr;
    QLineEdit* m_adminPin = nullptr;
    QSpinBox* m_adminBalance = nullptr;
    QPushButton* m_createAccountButton = nullptr;
    QPushButton* m_setBalanceButton = nullptr;
    AdminWindow* m_adminWindow = nullptr;
    std::vector<QPushButton*> m_buttons;
    bool m_closing = false;
    bool m_readyToClose = false;
    bool m_withdrawalPending = false;
    std::thread m_joiner;
};
