#pragma once
#include "http.hpp"
#include <QJsonObject>
#include <QDateTime>
#include <chrono>
#include <memory>
class QLockFile;
namespace hhc {
struct SyncState {
	QString recordingId, captureId, state, liveState, autoPublish;
	bool liveEnabled = false, stopAccepted = false, sealAccepted = false;
	quint64 pendingBytes = 0;
	int lastSequence = -1;
};
class CaptureSync {
public:
	using PutTransport = std::function<HttpResponse(const QByteArray &, const QUrl &, const QByteArray &,
							const QMap<QByteArray, QByteArray> &)>;
	CaptureSync(QString root, QString account, QString localId, ApiClient &, PutTransport put = httpRequest);
	SyncState begin(const QString &title, bool autoPublish, bool liveEnabled);
	SyncState step(bool encoderActive);
	SyncState control(bool closeLive);
	QString directory() const;
	static void persistControl(const QString &root, const QString &account, const QString &id, bool closeLive);

private:
	void load();
	std::unique_ptr<QLockFile> lock();
	SyncState beginImpl(const QString &, bool, bool);
	SyncState stepImpl(bool);
	SyncState controlImpl(bool);
	QJsonObject mutate(QString tag, QByteArray method, QString path, QJsonObject body, QString operation);
	QJsonObject poll();
	void apply(const QJsonObject &);
	void save();
	QString root_, account_, id_;
	ApiClient &api_;
	PutTransport put_;
	QJsonObject journal_;
	SyncState state_;
	std::chrono::steady_clock::time_point deadline_ = std::chrono::steady_clock::time_point::max();
};
} // namespace hhc
