#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include <atomic>
#include "capture-policy.hpp"
namespace hhc {
struct ClosedObject {
	QString path;
	qint64 size = 0;
	QString sha256;
	bool confirmed = false;
};
struct CaptureJournal {
	QString account, localId, remoteCapture, packageId, readyPackage;
	QVector<ClosedObject> objects;
	bool stopIntent = false, normalEnd = false, sealAcknowledged = false, confirmedReady = false;
	QDateTime readyAt;
};
struct RecoveryIssue {
	QString localId, message;
};
struct RecoveryReport {
	QVector<CaptureJournal> sessions;
	QVector<RecoveryIssue> issues;
};
class SessionStore {
public:
	explicit SessionStore(QString root);
	void save(const CaptureJournal &);
	// Single writer per session. Only newly appended bytes are hashed during capture;
	// recovery and successful finalization revalidate all immutable objects.
	void checkpointLocal(const QString &account, const QString &localId, const QVector<ClosedObject> &closed,
			     bool stopIntent, bool normalEnd);
	// Called only after a validated seal receipt and authoritative ready response.
	void confirmReady(const QString &account, const QString &localId, const QString &captureId,
			  const QString &packageId, QDateTime observedAt);
	QVector<CaptureJournal> loadPending(const QString &account) const;
	RecoveryReport scanPending(const QString &account, const std::atomic<bool> *cancelled = nullptr) const;
	QString mediaDirectory(const QString &account, const QString &localId) const;
	bool isPrepared(const QString &account, const QString &localId) const;
	bool mayCleanup(const CaptureJournal &, QDateTime now) const;

private:
	QString root_;
};
} // namespace hhc
