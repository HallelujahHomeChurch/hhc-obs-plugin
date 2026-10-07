#pragma once
#include <QWidget>
#include <QString>
#include <optional>
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
namespace hhc {
enum class Phase { Unavailable, Ready, Capturing, StopPending, Uploading, Validating, DraftReady, Published, Failed };
struct DockState {
	Phase phase = Phase::Unavailable;
	bool canPublish = false, liveEnabled = false, liveConfirmed = false, cancelPending = false;
	std::optional<bool> autoPublish;
	quint64 pendingBytes = 0;
	QString issue;
};
class Dock : public QWidget {
public:
	explicit Dock(QWidget *parent = nullptr);
	void apply(const DockState &);

private:
	QLabel *status_ = nullptr, *liveStatus_ = nullptr, *warning_ = nullptr, *pending_ = nullptr;
	QCheckBox *live_ = nullptr, *publish_ = nullptr;
	QLineEdit *title_ = nullptr;
	QPushButton *action_ = nullptr;
};
} // namespace hhc
