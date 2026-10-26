#pragma once
#include <QObject>
#include <QString>

class QtDisplayBridge : public QObject
{
    Q_OBJECT
public:
    explicit QtDisplayBridge(QObject* parent = nullptr);
Q_SIGNALS:
    void lineReady(QString line);
public:
    void emitLine(QString const& line);
};
