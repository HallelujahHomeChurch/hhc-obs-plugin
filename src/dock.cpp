#include "dock.hpp"
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QDate>
namespace hhc {
Dock::Dock(QWidget *parent) : QWidget(parent)
{
	setMinimumWidth(320);
	resize(360, 620);
	setWindowTitle(QString::fromUtf8("HHC 影音 — 介面測試"));
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(12, 12, 12, 12);
	outer->setSpacing(12);
	auto *note = new QLabel(QString::fromUtf8("介面測試 · 未連接 HHC 服務"));
	note->setWordWrap(true);
	outer->addWidget(note);
	auto *scroll = new QScrollArea;
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll, 1);
	auto *body = new QWidget;
	scroll->setWidget(body);
	auto *layout = new QVBoxLayout(body);
	layout->setContentsMargins(0, 0, 4, 0);
	layout->setSpacing(12);
	auto *top = new QHBoxLayout;
	auto *heading = new QLabel(QString::fromUtf8("HHC 影音"));
	auto font = heading->font();
	font.setBold(true);
	heading->setFont(font);
	top->addWidget(heading);
	top->addStretch();
	auto *settings = new QPushButton(QString::fromUtf8("設定"));
	settings->setEnabled(false);
	top->addWidget(settings);
	layout->addLayout(top);
	auto *account = new QLabel(QString::fromUtf8("帳號：測試狀態（尚未登入）"));
	account->setWordWrap(true);
	layout->addWidget(account);
	auto *titleLabel = new QLabel(QString::fromUtf8("本場標題"));
	title_ = new QLineEdit;
	title_->setObjectName("title");
	title_->setAccessibleName(titleLabel->text());
	title_->setMaxLength(180);
	title_->setText(QDate::currentDate().toString("yyyy-MM-dd") + QString::fromUtf8(" 聚會"));
	titleLabel->setBuddy(title_);
	layout->addWidget(titleLabel);
	layout->addWidget(title_);
	auto *source = new QLabel(QString::fromUtf8("Program · 音軌 1\n1080p／720p／480p · 29.97 fps"));
	source->setWordWrap(true);
	layout->addWidget(source);
	live_ = new QCheckBox(QString::fromUtf8("同步開放會員直播"));
	live_->setObjectName("live");
	layout->addWidget(live_);
	publish_ = new QCheckBox(QString::fromUtf8("完成後自動發布"));
	publish_->setObjectName("publish");
	layout->addWidget(publish_);
	auto *hint = new QLabel(
		QString::fromUtf8("請明確選擇發布意向。自動發布須等影片完整且驗證通過；直播與會後影片分開管理。"));
	hint->setWordWrap(true);
	layout->addWidget(hint);
	status_ = new QLabel;
	status_->setObjectName("status");
	status_->setWordWrap(true);
	status_->setFont(font);
	layout->addWidget(status_);
	liveStatus_ = new QLabel;
	liveStatus_->setWordWrap(true);
	layout->addWidget(liveStatus_);
	pending_ = new QLabel;
	pending_->setWordWrap(true);
	layout->addWidget(pending_);
	warning_ = new QLabel;
	warning_->setObjectName("warning");
	warning_->setWordWrap(true);
	warning_->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(warning_);
	auto *recent = new QGroupBox(QString::fromUtf8("最近收錄"));
	auto *recentLayout = new QVBoxLayout(recent);
	auto *empty = new QLabel(QString::fromUtf8("尚無已確認的收錄。\n未完成的本機資料會保留。"));
	empty->setWordWrap(true);
	recentLayout->addWidget(empty);
	layout->addWidget(recent);
	layout->addStretch();
	action_ = new QPushButton;
	action_->setObjectName("action");
	action_->setAutoDefault(false);
	action_->setDefault(false);
	outer->addWidget(action_);
	setTabOrder(title_, live_);
	setTabOrder(live_, publish_);
	setTabOrder(publish_, action_);
	apply({});
}
void Dock::apply(const DockState &s)
{
	live_->setChecked(s.liveEnabled);
	live_->setEnabled(s.canPublish && s.phase == Phase::Ready);
	publish_->setTristate(!s.autoPublish.has_value());
	publish_->setCheckState(s.autoPublish ? (*s.autoPublish ? Qt::Checked : Qt::Unchecked) : Qt::PartiallyChecked);
	publish_->setEnabled(s.canPublish && s.phase == Phase::Ready);
	title_->setEnabled(s.phase == Phase::Ready || s.phase == Phase::Unavailable);
	liveStatus_->setText(s.liveConfirmed ? QString::fromUtf8("會員直播中")
			     : s.liveEnabled ? QString::fromUtf8("直播開放狀態待確認")
					     : QString::fromUtf8("會員直播未開放"));
	pending_->setText(QString::fromUtf8("待傳：%1 MB · 三畫質").arg(double(s.pendingBytes) / 1000000.0, 0, 'f', 1));
	warning_->setText(s.cancelPending ? QString::fromUtf8("取消自動發布待伺服器確認") : s.issue);
	warning_->setVisible(!warning_->text().isEmpty());
	action_->setText(QString::fromUtf8("開始收錄"));
	action_->setEnabled(false);
	switch (s.phase) {
	case Phase::Unavailable:
		status_->setText(QString::fromUtf8("尚未連接 HHC 服務"));
		break;
	case Phase::Ready:
		status_->setText(QString::fromUtf8("準備收錄"));
		action_->setEnabled(s.autoPublish.has_value());
		break;
	case Phase::Capturing:
		status_->setText(QString::fromUtf8("收錄中"));
		action_->setText(QString::fromUtf8("停止收錄"));
		action_->setEnabled(true);
		break;
	case Phase::StopPending:
		status_->setText(QString::fromUtf8("已停止收錄，停止通知待同步"));
		action_->setText(QString::fromUtf8("停止通知待同步"));
		break;
	case Phase::Uploading:
		status_->setText(QString::fromUtf8("補傳中，請保持 OBS 開啟"));
		action_->setText(QString::fromUtf8("補傳中"));
		break;
	case Phase::Validating:
		status_->setText(QString::fromUtf8("伺服器檢查影片中"));
		action_->setText(QString::fromUtf8("等待檢查結果"));
		break;
	case Phase::DraftReady:
		status_->setText(QString::fromUtf8("草稿已就緒"));
		action_->setText(QString::fromUtf8("發布影片"));
		action_->setEnabled(s.canPublish);
		break;
	case Phase::Published:
		status_->setText(QString::fromUtf8("會後影片已發布 · 會員可觀看"));
		action_->setText(QString::fromUtf8("開啟影音專區"));
		action_->setEnabled(true);
		break;
	case Phase::Failed:
		status_->setText(QString::fromUtf8("收錄需要處理，本機資料已保留"));
		action_->setText(QString::fromUtf8("查看未完成收錄"));
		break;
	}
}
} // namespace hhc
