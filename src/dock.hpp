#pragma once
#include <QWidget>
#include <QString>
#include <optional>
#include <functional>
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QComboBox;
namespace hhc {
enum class Phase {
	Unavailable,
	Ready,
	Capturing,
	StopPending,
	Uploading,
	Validating,
	DraftReady,
	Published,
	Failed,
	LocalComplete
};
struct DockState {
	Phase phase = Phase::Unavailable;
	bool canPublish = false, liveEnabled = false, liveConfirmed = false, cancelPending = false;
	std::optional<bool> autoPublish;
	quint64 pendingBytes = 0;
	QString issue;
	bool localOnly = false;
	bool checkingLocal = false;
};
class Dock : public QWidget {
public:
	explicit Dock(QWidget *parent = nullptr);
	void apply(const DockState &);
	unsigned audioTrack() const;
	QString title() const;
	void setRecoveryText(const QString &);
	std::function<void()> onAction, onRefresh, onOpenFolder;

private:
	QLabel *status_ = nullptr, *liveStatus_ = nullptr, *warning_ = nullptr, *pending_ = nullptr;
	QCheckBox *live_ = nullptr, *publish_ = nullptr;
	QLineEdit *title_ = nullptr;
	QPushButton *action_ = nullptr;
	QLabel *note_ = nullptr, *recent_ = nullptr, *hint_ = nullptr;
	QComboBox *track_ = nullptr;
};
} // namespace hhc
