#include <QApplication>
#include <QFont>

#include "app/main_window.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setStyle(QStringLiteral("Fusion"));
    app.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    app.setStyleSheet(QStringLiteral(
        "QWidget { background: #f3f6f8; color: #23313b; }"
        "QMainWindow { background: #f3f6f8; }"
        "QLabel#eyebrow { color: #60717d; font-size: 10px; font-weight: 700; }"
        "QLabel#screen { background: #172b34; color: #e6f5ef; border: 1px solid #28434c; border-radius: 8px; padding: 18px; selection-background-color: #197c70; }"
        "QLineEdit, QSpinBox { background: #ffffff; border: 1px solid #cbd5dc; border-radius: 5px; padding: 8px 10px; min-height: 20px; selection-background-color: #16877a; }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 0; border: none; background: transparent; }"
        "QSpinBox::up-arrow, QSpinBox::down-arrow { width: 0; height: 0; }"
        "QLineEdit:focus, QSpinBox:focus { border: 1px solid #16877a; }"
        "QLineEdit:read-only, QSpinBox:disabled { background: #e9eef1; color: #7b8991; }"
        "QPushButton { background: #ffffff; border: 1px solid #cbd5dc; border-radius: 5px; padding: 9px 12px; min-height: 20px; font-weight: 600; }"
        "QPushButton:hover { background: #e8f2f0; border-color: #8cb8b0; }"
        "QPushButton:pressed { background: #d8e9e5; }"
        "QPushButton:disabled { color: #9aa5ab; background: #edf0f2; border-color: #d9dfe3; }"
        "QPushButton#primaryButton { background: #147d70; border-color: #147d70; color: white; }"
        "QPushButton#primaryButton:hover { background: #10695f; }"
        "QPushButton#dangerButton { color: #a3473b; }"
        "QPlainTextEdit { background: #ffffff; border: 1px solid #d4dde2; border-radius: 6px; padding: 8px; color: #43535d; }"
        "QTableWidget { background: #ffffff; alternate-background-color: #f5f8f9; border: 1px solid #d4dde2; border-radius: 5px; gridline-color: #e5eaed; selection-background-color: #d9efeb; selection-color: #193c39; }"
        "QHeaderView::section { background: #eaf0f2; color: #42545e; border: 0; border-bottom: 1px solid #d4dde2; padding: 8px; font-weight: 700; }"
        "QLabel#status { color: #52716d; padding: 2px 3px; }"));
    MainWindow w;
    w.show();
    return app.exec();
}
