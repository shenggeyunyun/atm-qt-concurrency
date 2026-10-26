#pragma once

#include <QMainWindow>
#include "core/bank_database.hpp"

class QLineEdit;
class QSpinBox;
class QLabel;
class QTableWidget;
class QTableWidgetItem;
class QPushButton;

class AdminWindow : public QMainWindow
{
public:
    explicit AdminWindow(QWidget* parent = nullptr);

private:
    QLineEdit* m_account = nullptr;
    QLineEdit* m_pin = nullptr;
    QSpinBox* m_balance = nullptr;
    QLabel* m_status = nullptr;
    QTableWidget* m_accounts = nullptr;
    bank_database m_database;
    QPushButton* m_delete = nullptr;
    void refreshAccounts();
    void updateBalance(QTableWidgetItem* item);
    void updatePin(QTableWidgetItem* item);
};
