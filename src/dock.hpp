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
	Creating,
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
	bool canPublish = false, liveEnabled = false, cancelPending = false;
	QString liveState;
	std::optional<bool> autoPublish;
	quint64 pendingBytes = 0;
	QString issue;
	bool localOnly = false;
	bool connected = false;
	QString account;
	bool checkingLocal = false;
	bool terminalFailure = false;
};
class Dock : public QWidget {
public:
	explicit Dock(QWidget *parent = nullptr);
	void apply(const DockState &);
	unsigned audioTrack() const;
	QString title() const;
	bool selectedLive() const;
	bool selectedPublish() const;
	void setRecoverySessions(const QStringList &, const QStringList &cleanupIds = {});
	void setRecoveryText(const QString &);
	std::function<void()> onAction, onRefresh, onOpenFolder, onLogin, onCloseLive, onCancelPublish;
	std::function<void(QString)> onRecover, onOpenCleanup;

private:
	QLabel *status_ = nullptr, *liveStatus_ = nullptr, *warning_ = nullptr, *pending_ = nullptr;
	QCheckBox *live_ = nullptr, *publish_ = nullptr;
	QLineEdit *title_ = nullptr;
	QPushButton *action_ = nullptr;
	QLabel *note_ = nullptr, *recent_ = nullptr, *hint_ = nullptr;
	QComboBox *track_ = nullptr, *sessions_ = nullptr;
	QPushButton *login_ = nullptr, *closeLive_ = nullptr, *cancelPublish_ = nullptr;
	QLabel *account_ = nullptr;
	QPushButton *cleanup_ = nullptr;
	bool allowCleanup_ = false;
	bool capturing_ = false, localOnly_ = false;
};
} // namespace hhc
