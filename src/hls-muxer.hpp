#pragma once
#include "capture-policy.hpp"
#include <obs.h>
#include <QJsonArray>
#include <QString>
#include <memory>
namespace hhc {
class HlsMuxer {
public:
	HlsMuxer(const QString &root, unsigned rendition, obs_encoder_t *video, obs_encoder_t *audio);
	~HlsMuxer();
	void write(const encoder_packet &);
	void close();
	QJsonArray objects() const;

private:
	struct Impl;
	std::unique_ptr<Impl> d;
	void publishClosed();
};
void atomicJson(const QString &path, const QJsonObject &);
bool tryObserverJson(const QString &path, const QJsonObject &);
QJsonObject finalizeMaster(const QString &root);
} // namespace hhc
