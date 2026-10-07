#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include "capture-policy.hpp"
namespace hhc {
struct ClosedObject { QString path; qint64 size=0; QString sha256; bool confirmed=false; };
struct CaptureJournal {
 QString account,localId,remoteCapture,packageId,readyPackage;
 QVector<ClosedObject> objects;
 bool stopIntent=false,normalEnd=false,sealAcknowledged=false,confirmedReady=false;
 QDateTime readyAt;
};
class SessionStore {
public:
 explicit SessionStore(QString root);
 void save(const CaptureJournal&);
 QVector<CaptureJournal> loadPending(const QString& account) const;
 QString mediaDirectory(const QString& account,const QString& localId) const;
 bool mayCleanup(const CaptureJournal&,QDateTime now) const;
private: QString root_;
};
}
