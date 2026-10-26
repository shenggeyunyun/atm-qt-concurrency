#include "app/qt_display_bridge.hpp"
#include <QString>

QtDisplayBridge::QtDisplayBridge(QObject* parent)
    : QObject(parent)
{ }

void QtDisplayBridge::emitLine(QString const& line)
{
    emit lineReady(line);
}
