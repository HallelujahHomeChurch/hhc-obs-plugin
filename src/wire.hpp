#pragma once
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>
namespace hhc {
QString initCodecTag(const QByteArray &avcc, const QByteArray &aac);
qint64 measuredBandwidth(const QVector<double> &, const QVector<qint64> &, double targetDuration);
bool validWire(const QString &schema, const QJsonValue &value);
QByteArray canonicalInventory(const QJsonObject &inventory);
QJsonObject buildInventory(const QString &directory);
} // namespace hhc
