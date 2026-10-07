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
#include <QComboBox>
namespace hhc {
Dock::Dock(QWidget *parent) : QWidget(parent)
{
	setMinimumWidth(320);
	setObjectName("hhcCaptureDock");
	resize(360, 620);
	setWindowTitle(QString::fromUtf8("HHC 影音 — 介面測試"));
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(12, 12, 12, 12);
	outer->setSpacing(12);
	auto *note = new QLabel(QString::fromUtf8("介面測試 · 未連接 HHC 服務"));
	note->setWordWrap(true);
	outer->addWidget(note);
	note_ = note;
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
	account_ = account;
	login_ = new QPushButton(QString::fromUtf8("登入 HHC"));
	login_->setObjectName("login");
	layout->addWidget(login_);
	connect(login_, &QPushButton::clicked, this, [this] {
		if (onLogin)
			onLogin();
	});
	auto *titleLabel = new QLabel(QString::fromUtf8("本場標題"));
	title_ = new QLineEdit;
	title_->setObjectName("title");
	title_->setAccessibleName(titleLabel->text());
	title_->setMaxLength(180);
	title_->setText(QDate::currentDate().toString("yyyy-MM-dd") + QString::fromUtf8(" 聚會"));
	titleLabel->setBuddy(title_);
	layout->addWidget(titleLabel);
	layout->addWidget(title_);
	auto *source = new QLabel(QString::fromUtf8("Program · 1080p／720p／480p · 29.97 fps"));
	source->setWordWrap(true);
	layout->addWidget(source);
	track_ = new QComboBox;
	track_->setObjectName("audioTrack");
	track_->setAccessibleName(QString::fromUtf8("收錄音軌"));
	for (int i = 1; i <= 6; ++i)
		track_->addItem(QString::fromUtf8("音軌 %1").arg(i), i);
	layout->addWidget(track_);
	live_ = new QCheckBox(QString::fromUtf8("同步開放會員直播"));
	live_->setObjectName("live");
	layout->addWidget(live_);
	publish_ = new QCheckBox(QString::fromUtf8("完成後自動發布"));
	publish_->setObjectName("publish");
	layout->addWidget(publish_);
	closeLive_ = new QPushButton(QString::fromUtf8("關閉會員直播，繼續錄影"));
	closeLive_->setObjectName("closeLive");
	layout->addWidget(closeLive_);
	connect(closeLive_, &QPushButton::clicked, this, [this] {
		if (onCloseLive)
			onCloseLive();
	});
	cancelPublish_ = new QPushButton(QString::fromUtf8("取消會後自動發布"));
	cancelPublish_->setObjectName("cancelPublish");
	layout->addWidget(cancelPublish_);
	connect(cancelPublish_, &QPushButton::clicked, this, [this] {
		if (onCancelPublish)
			onCancelPublish();
	});
	auto *hint = new QLabel(
		QString::fromUtf8("請明確選擇發布意向。自動發布須等影片完整且驗證通過；直播與會後影片分開管理。"));
	hint->setWordWrap(true);
	layout->addWidget(hint);
	hint_ = hint;
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
	empty->setTextFormat(Qt::PlainText);
	empty->setObjectName("recovery");
	recent_ = empty;
	auto *refresh = new QPushButton(QString::fromUtf8("重新檢查本機收錄"));
	refresh->setObjectName("refreshRecovery");
	recentLayout->addWidget(refresh);
	auto *folder = new QPushButton(QString::fromUtf8("開啟本機資料夾"));
	folder->setObjectName("openFolder");
	recentLayout->addWidget(folder);
	sessions_ = new QComboBox;
	sessions_->setObjectName("recoverSession");
	recentLayout->addWidget(sessions_);
	auto *recover = new QPushButton(QString::fromUtf8("繼續同步所選收錄"));
	recover->setObjectName("resumeSession");
	recentLayout->addWidget(recover);
	connect(recover, &QPushButton::clicked, this, [this] {
		if (onRecover && !sessions_->currentData().toString().isEmpty())
			onRecover(sessions_->currentData().toString());
	});
	connect(refresh, &QPushButton::clicked, this, [this] {
		if (onRefresh)
			onRefresh();
	});
	connect(folder, &QPushButton::clicked, this, [this] {
		if (onOpenFolder)
			onOpenFolder();
	});
	layout->addWidget(recent);
	layout->addStretch();
	action_ = new QPushButton;
	action_->setObjectName("action");
	action_->setAutoDefault(false);
	action_->setDefault(false);
	outer->addWidget(action_);
	connect(action_, &QPushButton::clicked, this, [this] {
		if (onAction)
			onAction();
	});
	setTabOrder(title_, live_);
	setTabOrder(live_, publish_);
	setTabOrder(publish_, action_);
	apply({});
}
void Dock::apply(const DockState &s)
{
	const bool idle = s.phase == Phase::Ready || s.phase == Phase::DraftReady || s.phase == Phase::Published ||
			  s.phase == Phase::Failed || s.phase == Phase::Unavailable;
	login_->setVisible(!s.localOnly);
	login_->setEnabled(idle && !s.checkingLocal);
	login_->setText(s.account.isEmpty() ? QString::fromUtf8("登入 HHC") : QString::fromUtf8("重新登入 HHC"));
	account_->setText(s.account.isEmpty() ? QString::fromUtf8("尚未登入")
					      : QString::fromUtf8("帳號：%1").arg(s.account));
	closeLive_->setVisible(!s.localOnly && s.connected);
	cancelPublish_->setVisible(!s.localOnly && s.connected);
	closeLive_->setEnabled(s.liveEnabled && (s.phase == Phase::Capturing || s.phase == Phase::Uploading));
	cancelPublish_->setEnabled(
		s.autoPublish.value_or(false) &&
		(s.phase == Phase::Capturing || s.phase == Phase::Uploading || s.phase == Phase::Validating));
	sessions_->setVisible(!s.localOnly);
	sessions_->setEnabled(idle && !s.checkingLocal);
	if (s.connected)
		hint_->setText(QString::fromUtf8(
			"直播與會後發布各自獨立。只錄影請保持兩項關閉；停止後會繼續補傳並等待伺服器驗證。"));
	if (s.localOnly)
		hint_->setText(QString::fromUtf8("本機驗證不會建立平台影音，資料不會自動綁定未來登入的帳號。"));
	note_->setText(s.localOnly ? QString::fromUtf8("本機驗證模式 · 不上傳、不開放直播、不發布。平台連線尚未設定。")
		       : s.connected ? QString::fromUtf8("HHC 正式平台 · 29.97 fps")
				     : QString::fromUtf8("尚未登入 HHC 正式平台"));
	track_->setEnabled(s.phase == Phase::Ready || s.phase == Phase::LocalComplete || s.phase == Phase::Failed);
	live_->setChecked(s.liveEnabled);
	live_->setEnabled(!s.localOnly && s.canPublish && s.phase == Phase::Ready);
	publish_->setTristate(!s.autoPublish.has_value());
	publish_->setCheckState(s.autoPublish ? (*s.autoPublish ? Qt::Checked : Qt::Unchecked) : Qt::PartiallyChecked);
	publish_->setEnabled(!s.localOnly && s.canPublish && s.phase == Phase::Ready);
	title_->setEnabled(s.phase == Phase::Ready || s.phase == Phase::Unavailable ||
			   s.phase == Phase::LocalComplete || s.phase == Phase::Failed);
	liveStatus_->setText(s.liveConfirmed ? QString::fromUtf8("會員直播中")
			     : s.liveEnabled ? QString::fromUtf8("直播開放狀態待確認")
					     : QString::fromUtf8("會員直播未開放"));
	pending_->setText(QString::fromUtf8("待傳：%1 MB · 三畫質").arg(double(s.pendingBytes) / 1000000.0, 0, 'f', 1));
	if (s.localOnly)
		pending_->setText(QString::fromUtf8("本機編碼量：約 %1 MB · 三畫質")
					  .arg(double(s.pendingBytes) / 1000000.0, 0, 'f', 1));
	warning_->setText(s.cancelPending ? QString::fromUtf8("取消自動發布待伺服器確認") : s.issue);
	warning_->setVisible(!warning_->text().isEmpty());
	action_->setText(QString::fromUtf8("開始收錄"));
	action_->setEnabled(false);
	switch (s.phase) {
	case Phase::Creating:
		status_->setText(QString::fromUtf8("正在建立本場收錄，請稍候"));
		action_->setText(QString::fromUtf8("建立收錄中"));
		break;
	case Phase::Unavailable:
		status_->setText(s.connected ? QString::fromUtf8("已登入，但尚無收錄權限")
					     : QString::fromUtf8("尚未登入 HHC"));
		break;
	case Phase::Ready:
		status_->setText(QString::fromUtf8("準備收錄"));
		action_->setEnabled(s.localOnly || s.autoPublish.has_value());
		if (s.localOnly)
			action_->setText(QString::fromUtf8("開始本機驗證收錄"));
		break;
	case Phase::Capturing:
		status_->setText(QString::fromUtf8("收錄中"));
		action_->setText(QString::fromUtf8("停止收錄"));
		action_->setEnabled(true);
		break;
	case Phase::StopPending:
		status_->setText(QString::fromUtf8("已停止收錄，停止通知待同步"));
		action_->setText(QString::fromUtf8("停止通知待同步"));
		if (s.localOnly) {
			status_->setText(QString::fromUtf8("正在完成本機尾段與驗證，請保持 OBS 開啟"));
			action_->setText(QString::fromUtf8("本機收尾中"));
		}
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
		action_->setText(QString::fromUtf8("開始另一場收錄"));
		action_->setEnabled(s.connected);
		break;
	case Phase::Published:
		status_->setText(QString::fromUtf8("會後影片已發布 · 會員可觀看"));
		action_->setText(QString::fromUtf8("開始另一場收錄"));
		action_->setEnabled(true);
		break;
	case Phase::Failed:
		status_->setText(QString::fromUtf8("收錄需要處理，本機資料已保留"));
		action_->setText(QString::fromUtf8("查看未完成收錄"));
		if (s.localOnly) {
			action_->setText(QString::fromUtf8("開始另一場本機驗證"));
			action_->setEnabled(true);
		}
		break;
	case Phase::LocalComplete:
		status_->setText(QString::fromUtf8("本機收錄完成 · 尚未上傳或發布"));
		action_->setText(QString::fromUtf8("開始另一場本機驗證"));
		action_->setEnabled(true);
		break;
	}
	if (s.checkingLocal && s.phase != Phase::Capturing && s.phase != Phase::StopPending)
		action_->setEnabled(false);
}
unsigned Dock::audioTrack() const
{
	return track_->currentData().toUInt();
}
QString Dock::title() const
{
	return title_->text();
}
void Dock::setRecoveryText(const QString &text)
{
	recent_->setText(text);
}
} // namespace hhc

namespace hhc {
bool Dock::selectedLive() const
{
	return live_->isChecked();
}
bool Dock::selectedPublish() const
{
	return publish_->isChecked();
}
void Dock::setRecoverySessions(const QStringList &ids)
{
	auto selected = sessions_->currentData();
	sessions_->clear();
	for (const auto &id : ids)
		sessions_->addItem(id, id);
	auto i = sessions_->findData(selected);
	if (i >= 0)
		sessions_->setCurrentIndex(i);
}
} // namespace hhc
