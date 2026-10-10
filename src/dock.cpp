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
#include <QMessageBox>
#include <QPointer>
#include <QJsonObject>
#include <QSignalBlocker>
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
	logout_ = new QPushButton(QString::fromUtf8("登出"));
	logout_->setObjectName("logout");
	top->addWidget(logout_);
	connect(logout_, &QPushButton::clicked, this, [this] {
		if (onLogout)
			onLogout();
	});
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
	broadcasts_ = new QComboBox;
	broadcasts_->setObjectName("broadcasts");
	broadcasts_->setAccessibleName(QString::fromUtf8("收錄方式與直播場次"));
	setBroadcasts({});
	layout->addWidget(broadcasts_);
	broadcastRefresh_ = new QPushButton(QString::fromUtf8("重新查詢可綁定的直播"));
	broadcastRefresh_->setObjectName("broadcastRefresh");
	layout->addWidget(broadcastRefresh_);
	connect(broadcastRefresh_, &QPushButton::clicked, this, [this] {
		if (onBroadcastRefresh)
			onBroadcastRefresh();
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
	liveStatus_->setObjectName("liveStatus");
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
	cleanup_ = new QPushButton(QString::fromUtf8("查看可清理暫存"));
	cleanup_->setObjectName("openCleanupFolder");
	cleanup_->setToolTip(QString::fromUtf8("限已確認套件相符且滿七日的收錄。只開啟所選資料夾，不會刪除資料。"));
	recentLayout->addWidget(cleanup_);
	connect(cleanup_, &QPushButton::clicked, this, [this] {
		if (onOpenCleanup && sessions_->currentData(Qt::UserRole + 1).toBool())
			onOpenCleanup(sessions_->currentData().toString());
	});
	connect(sessions_, &QComboBox::currentIndexChanged, this,
		[this] { cleanup_->setEnabled(allowCleanup_ && sessions_->currentData(Qt::UserRole + 1).toBool()); });
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
		if (capturing_) {
			const QPointer<Dock> self(this);
			const auto message =
				localOnly_
					? QString::fromUtf8(
						  "停止這場 HHC 本機驗證收錄？完成後保留資料，不會上傳或發布。")
					: QString::fromUtf8("停止這場 HHC 收錄？停止後會繼續補傳並等待影片驗證。\n") +
						  (selectedPublish()
							   ? QString::fromUtf8("本場驗證通過後會自動發布。")
							   : QString::fromUtf8("本場保留為草稿，不會自動發布。"));
			if (QMessageBox::question(this, QString::fromUtf8("停止 HHC 收錄"), message,
						  QMessageBox::Yes | QMessageBox::Cancel,
						  QMessageBox::Cancel) != QMessageBox::Yes ||
			    !self || !self->capturing_)
				return;
		}
		if (onAction)
			onAction();
	});
	setTabOrder(title_, track_);
	setTabOrder(track_, live_);
	setTabOrder(live_, publish_);
	setTabOrder(publish_, action_);
	apply({});
	connect(broadcasts_, &QComboBox::currentIndexChanged, this, [this] {
		apply(currentState_);
		if (!selectedBroadcast().isEmpty()) {
			title_->setText(broadcasts_->currentText());
			hint_->setText(QString::fromUtf8(
			    "綁定後先收錄預覽。直播開始、結束與會後發布由控制室管理；結束直播會繼續收錄。"));
		}
	});
}
void Dock::apply(const DockState &s)
{
	currentState_ = s;
	if (!s.localOnly && accountId_ != s.account) {
		accountId_ = s.account;
		title_->setText(QDate::currentDate().toString("yyyy-MM-dd") + QString::fromUtf8(" 聚會"));
		track_->setCurrentIndex(0);
		setRecoverySessions({});
		setBroadcasts({});
		setRecoveryText(QString::fromUtf8("尚無此帳號的本機收錄。"));
	}
	capturing_ = s.phase == Phase::Capturing;
	localOnly_ = s.localOnly;
	const bool idle = s.phase == Phase::Ready || s.phase == Phase::DraftReady || s.phase == Phase::Published ||
			  s.phase == Phase::Failed || s.phase == Phase::Unavailable;
	broadcasts_->setVisible(!s.localOnly);
	broadcastRefresh_->setVisible(!s.localOnly);
	broadcasts_->setEnabled(s.connected && s.phase == Phase::Ready && !s.checkingLocal);
	broadcastRefresh_->setEnabled(s.connected && idle && !s.checkingLocal);
	if (s.broadcastBound && s.phase == Phase::Capturing)
		broadcastRefresh_->setEnabled(true);
	broadcastRefresh_->setText(
	    QString::fromUtf8(s.broadcastBound ? "重新確認直播控制" : "重新查詢可綁定的直播"));
	login_->setVisible(!s.localOnly);
	login_->setEnabled(idle && !s.checkingLocal);
	logout_->setVisible(!s.localOnly && s.connected);
	logout_->setEnabled(s.connected && idle && !s.checkingLocal);
	login_->setText(s.account.isEmpty() ? QString::fromUtf8("登入 HHC") : QString::fromUtf8("重新登入 HHC"));
	account_->setText(s.account.isEmpty() ? QString::fromUtf8("尚未登入")
					      : QString::fromUtf8("帳號：%1").arg(s.account));
	closeLive_->setVisible(!s.localOnly && s.connected && !s.broadcastBound);
	cancelPublish_->setVisible(!s.localOnly && s.connected && !s.broadcastBound);
	closeLive_->setEnabled(s.liveEnabled && (s.phase == Phase::Capturing || s.phase == Phase::Uploading));
	cancelPublish_->setEnabled(
		s.autoPublish.value_or(false) &&
		(s.phase == Phase::Capturing || s.phase == Phase::Uploading || s.phase == Phase::Validating));
	sessions_->setVisible(!s.localOnly);
	sessions_->setEnabled(idle && !s.checkingLocal);
	allowCleanup_ = !s.localOnly && s.connected && idle && !s.checkingLocal;
	cleanup_->setVisible(!s.localOnly);
	cleanup_->setEnabled(allowCleanup_ && sessions_->currentData(Qt::UserRole + 1).toBool());
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
	QString liveMessage = QString::fromUtf8("會員直播未開放");
	if (!s.localOnly) {
		if (s.liveEnabled)
			liveMessage = QString::fromUtf8("直播開放狀態待確認");
		for (const auto &[serverState, message] :
		     {std::pair{"starting", "會員直播準備中"}, std::pair{"live", "會員直播中"},
		      std::pair{"recovering", "會員直播補傳中"}, std::pair{"interrupted", "會員直播連線中斷"},
		      std::pair{"ending", "會員直播收尾中"}, std::pair{"ended", "會員直播已結束"},
		      std::pair{"failed", "會員直播失敗"}, std::pair{"expired", "會員直播已到期"},
		      std::pair{"aborted", "會員直播已中止"}})
			if (s.liveState == serverState) {
				liveMessage = s.liveEnabled
						      ? QString::fromUtf8(message) + QString::fromUtf8("（伺服器回報）")
						      : QString::fromUtf8("會員直播未開放；伺服器回報：") +
								QString::fromUtf8(message);
				break;
			}
	}
	liveStatus_->setText(liveMessage);
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
		if (s.terminalFailure) {
			action_->setText(QString::fromUtf8("準備另一場收錄"));
			action_->setEnabled(s.connected);
		}
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
	if (!s.localOnly && (s.broadcastBound || !selectedBroadcast().isEmpty())) {
		title_->setEnabled(false);
		live_->setEnabled(false);
		publish_->setEnabled(false);
		hint_->setText(QString::fromUtf8("控制室管理直播開始、結束與會後發布。停止收錄只停止本次 "
						 "HHC，不影響 YouTube 或本機錄影。"));
		if (s.phase == Phase::Ready)
			action_->setText(QString::fromUtf8("綁定並開始 HHC 收錄"));
		if (s.phase == Phase::Creating)
			status_->setText(QString::fromUtf8("等待直播綁定就緒"));
		if (s.broadcastBound) {
			QString label = QString::fromUtf8("待確認");
			for (const auto &[phase, text] :
			     {std::pair{"draft", "草稿"}, std::pair{"scheduled", "已發布預告"},
			      std::pair{"preview", "預覽收錄中"}, std::pair{"start_pending", "等待開播條件"},
			      std::pair{"live", "直播中"}, std::pair{"end_pending", "等待結束邊界"},
			      std::pair{"processing", "會後處理中"}, std::pair{"archived", "已轉為錄影"},
			      std::pair{"cancelled", "已取消"}, std::pair{"failed", "需要處理"}})
				if (s.broadcastPhase == phase)
					label = QString::fromUtf8(text);
			liveStatus_->setText(QString::fromUtf8("控制室：") + label);
		}
	}
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
QString Dock::selectedBroadcast() const { return broadcasts_->currentData().toString(); }
void Dock::setBroadcasts(const QJsonArray &items)
{
	const auto selected = broadcasts_->currentData();
	const QSignalBlocker blocker(broadcasts_);
	broadcasts_->clear();
	broadcasts_->addItem(QString::fromUtf8("獨立收錄（既有錄影／直播設定）"), QString{});
	for (const auto &v : items) {
		const auto o = v.toObject();
		broadcasts_->addItem(o["title"].toString(), o["recordingId"]);
	}
	const auto index = broadcasts_->findData(selected);
	if (index >= 0)
		broadcasts_->setCurrentIndex(index);
}
void Dock::setRecoverySessions(const QStringList &ids, const QStringList &cleanupIds)
{
	auto selected = sessions_->currentData();
	sessions_->clear();
	for (const auto &id : ids) {
		sessions_->addItem(id, id);
		sessions_->setItemData(sessions_->count() - 1, cleanupIds.contains(id), Qt::UserRole + 1);
	}
	auto i = sessions_->findData(selected);
	if (i >= 0)
		sessions_->setCurrentIndex(i);
	cleanup_->setEnabled(allowCleanup_ && sessions_->currentData(Qt::UserRole + 1).toBool());
}
} // namespace hhc
