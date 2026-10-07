#include "wire.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace hhc {
namespace {
const QJsonObject schemaRoot = QJsonDocument::fromJson(QByteArray(
#include "c1-schema.inc"
							       ))
				       .object();
QJsonObject resolve(const QString &ref)
{
	QJsonValue v(schemaRoot);
	for (const auto &key : ref.mid(2).split('/'))
		v = v.toObject()[key];
	return v.toObject();
}
bool validate(const QJsonObject &s, const QJsonValue &v, int depth = 0)
{
	if (depth > 40 || s.empty())
		return false;
	if (s.contains("$ref"))
		return validate(resolve(s["$ref"].toString()), v, depth + 1);
	for (const auto &a : s["allOf"].toArray())
		if (!validate(a.toObject(), v, depth + 1))
			return false;
	if (s.contains("oneOf")) {
		int matches = 0;
		for (const auto &a : s["oneOf"].toArray())
			matches += validate(a.toObject(), v, depth + 1);
		if (matches != 1)
			return false;
	}
	if (s.contains("const") && s["const"] != v)
		return false;
	if (s.contains("enum") && !s["enum"].toArray().contains(v))
		return false;
	auto types = s["type"].isArray() ? s["type"].toArray() : QJsonArray{s["type"]};
	bool typed = !s.contains("type");
	for (const auto &t : types) {
		const auto n = t.toString();
		typed |= (n == "object" && v.isObject()) || (n == "array" && v.isArray()) ||
			 (n == "string" && v.isString()) || (n == "boolean" && v.isBool()) ||
			 (n == "null" && v.isNull()) || (n == "number" && v.isDouble()) ||
			 (n == "integer" && v.isDouble() && std::floor(v.toDouble()) == v.toDouble());
	}
	if (!typed)
		return false;
	if (v.isObject()) {
		const auto o = v.toObject(), p = s["properties"].toObject();
		for (const auto &r : s["required"].toArray())
			if (!o.contains(r.toString()))
				return false;
		for (auto i = o.begin(); i != o.end(); ++i) {
			if (p.contains(i.key())) {
				if (!validate(p[i.key()].toObject(), i.value(), depth + 1))
					return false;
			} else if (s["additionalProperties"].isBool() && !s["additionalProperties"].toBool())
				return false;
			else if (s["additionalProperties"].isObject() &&
				 !validate(s["additionalProperties"].toObject(), i.value(), depth + 1))
				return false;
		}
	}
	if (v.isArray()) {
		auto a = v.toArray();
		if (a.size() < s["minItems"].toInt() || (s.contains("maxItems") && a.size() > s["maxItems"].toInt()))
			return false;
		for (int i = 0; i < a.size(); ++i) {
			if (s.contains("items") && !validate(s["items"].toObject(), a[i], depth + 1))
				return false;
			if (s["uniqueItems"].toBool())
				for (int j = 0; j < i; ++j)
					if (a[i] == a[j])
						return false;
		}
	}
	if (v.isString()) {
		auto str = v.toString();
		if (str.size() < s["minLength"].toInt() ||
		    (s.contains("maxLength") && str.size() > s["maxLength"].toInt()))
			return false;
		if (s.contains("pattern") &&
		    !QRegularExpression("\\A(?:" + s["pattern"].toString() + ")\\z").match(str).hasMatch())
			return false;
		if (s["format"] == "uuid" &&
		    !QRegularExpression(
			     "\\A[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\z")
			     .match(str)
			     .hasMatch())
			return false;
		if (s["format"] == "date-time" &&
		    (!str.endsWith('Z') || !QDateTime::fromString(str, Qt::ISODateWithMs).isValid()))
			return false;
	}
	if (v.isDouble()) {
		double n = v.toDouble();
		if (!std::isfinite(n))
			return false;
		if (s.contains("minimum") && n < s["minimum"].toDouble())
			return false;
		if (s.contains("maximum") && n > s["maximum"].toDouble())
			return false;
		if (s.contains("exclusiveMinimum") && n <= s["exclusiveMinimum"].toDouble())
			return false;
		if (s.contains("multipleOf") && std::fmod(n, s["multipleOf"].toDouble()) != 0)
			return false;
	}
	return true;
}
QByteArray valueBytes(const QJsonValue &v)
{
	if (v.isDouble()) {
		char buf[128];
		auto r = std::to_chars(buf, buf + 128, v.toDouble(),
				       (std::abs(v.toDouble()) >= 1e-6 && std::abs(v.toDouble()) < 1e21) ||
						       v.toDouble() == 0
					       ? std::chars_format::fixed
					       : std::chars_format::general);
		return QByteArray(buf, int(r.ptr - buf));
	}
	auto b = QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact);
	return b.mid(1, b.size() - 2);
}
QByteArray ordered(const QJsonObject &o, const QStringList &fields)
{
	QByteArray b = "{";
	for (const auto &key : fields) {
		if (b.size() > 1)
			b += ',';
		b += valueBytes(key) + ':' + valueBytes(o[key]);
	}
	return b + '}';
}
QByteArray sortedArray(QJsonArray a, const char *key, const QStringList &fields)
{
	QVector<QJsonValue> values;
	for (const auto &v : a)
		values.append(v);
	std::sort(values.begin(), values.end(), [key](const auto &x, const auto &y) {
		return x.toObject()[key].toString() < y.toObject()[key].toString();
	});
	QByteArray b = "[";
	for (const auto &v : values) {
		if (b.size() > 1)
			b += ',';
		b += ordered(v.toObject(), fields);
	}
	return b + ']';
}
} // namespace
QString initCodecTag(const QByteArray &avcc, const QByteArray &aac)
{
	if (avcc.size() < 4 || quint8(avcc[0]) != 1 || aac.size() < 2 || (quint8(aac[0]) >> 3) != 2)
		throw std::runtime_error("Unsupported init codec configuration");
	return "avc1." + QString::fromLatin1(avcc.mid(1, 3).toHex()) + ",mp4a.40.2";
}
qint64 measuredBandwidth(const QVector<double> &durations, const QVector<qint64> &sizes, double target)
{
	if (durations.empty() || durations.size() != sizes.size() || target <= 0)
		throw std::runtime_error("Invalid bandwidth timeline");
	double peak = 0, totalDuration = 0;
	qint64 totalBytes = 0;
	for (int i = 0; i < durations.size(); ++i) {
		totalDuration += durations[i];
		totalBytes += sizes[i];
		double seconds = 0;
		qint64 bytes = 0;
		for (int j = i; j < durations.size(); ++j) {
			seconds += durations[j];
			bytes += sizes[j];
			if (seconds > target * 1.5)
				break;
			if (seconds >= target * .5)
				peak = std::max(peak, 8.0 * bytes / seconds);
		}
	}
	if (peak == 0)
		peak = 8.0 * totalBytes / totalDuration;
	return qint64(std::ceil(peak));
}
bool validWire(const QString &schema, const QJsonValue &value)
{
	return validate(resolve("#/components/schemas/" + schema), value);
}
QByteArray canonicalInventory(const QJsonObject &i)
{
	return "{\"schemaVersion\":" + valueBytes(i["schemaVersion"]) +
	       ",\"presetVersion\":" + valueBytes(i["presetVersion"]) +
	       ",\"objects\":" + sortedArray(i["objects"].toArray(), "path", {"path", "sizeBytes", "sha256"}) +
	       ",\"renditions\":" +
	       sortedArray(i["renditions"].toArray(), "name",
			   {"name", "width", "height", "frameRate", "videoBitrate", "audioBitrate", "durationSeconds",
			    "segmentCount"}) +
	       '}';
}
QJsonObject buildInventory(const QString &dir)
{
	QFile f(dir + "/inventory.json");
	if (!f.open(QIODevice::ReadOnly) || f.size() > 8 * 1024 * 1024)
		throw std::runtime_error("Final inventory unavailable");
	auto local = QJsonDocument::fromJson(f.readAll()).object();
	if (!local["normalEnd"].toBool())
		throw std::runtime_error("Incomplete media cannot seal");
	QJsonArray objects, renditions;
	for (const auto &v : local["objects"].toArray()) {
		auto o = v.toObject();
		objects.append(QJsonObject{{"path", o["path"]}, {"sizeBytes", o["size"]}, {"sha256", o["sha256"]}});
	}
	for (const auto &name : QStringList{"1080p", "720p", "480p"}) {
		QFile p(dir + "/" + name + "/index.m3u8");
		if (!p.open(QIODevice::ReadOnly))
			throw std::runtime_error("Final playlist unavailable");
		auto text = QString::fromUtf8(p.readAll());
		if (!text.contains("#EXT-X-ENDLIST"))
			throw std::runtime_error("Incomplete media cannot seal");
		auto matches = QRegularExpression("#EXTINF:([0-9.]+)").globalMatch(text);
		int count = 0;
		double seconds = 0;
		while (matches.hasNext()) {
			seconds += matches.next().captured(1).toDouble();
			++count;
		}
		int h = name.chopped(1).toInt(),
		    w = h == 480   ? 854
			: h == 720 ? 1280
				   : 1920,
		    bitrate = h == 480   ? 800000
			      : h == 720 ? 1500000
					 : 3000000;
		renditions.append(QJsonObject{{"name", name},
					      {"width", w},
					      {"height", h},
					      {"frameRate", 30000.0 / 1001},
					      {"videoBitrate", bitrate},
					      {"audioBitrate", 128000},
					      {"durationSeconds", seconds},
					      {"segmentCount", count}});
	}
	QJsonObject result{{"schemaVersion", 1},
			   {"presetVersion", "hhc-obs-v1"},
			   {"objects", objects},
			   {"renditions", renditions}};
	result["inventoryDigest"] = QString::fromLatin1(
		QCryptographicHash::hash(canonicalInventory(result), QCryptographicHash::Sha256).toHex());
	if (!validWire("RecordingPackageInventory", result))
		throw std::runtime_error("Canonical inventory invalid");
	return result;
}
} // namespace hhc
