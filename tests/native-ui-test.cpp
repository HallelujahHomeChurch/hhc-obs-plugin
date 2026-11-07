#include "native-ui.hpp"
#include <QTemporaryDir>
#include <iostream>
#include <memory>

namespace hhc {
struct KeyControllerTest {
 static void setFinalizing(KeyController &controller, bool released) {
  controller.timer_.stop();
  controller.id_ = "test-output";
  controller.released_ = released;
 }
};
}

class MockSettings : public QDialog {
 Q_OBJECT
public:
 QListWidget *list = new QListWidget(this);
 QStackedWidget *pages = new QStackedWidget(this);
 QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
 int nativeCalls = 0;
 bool rejectApply = false;
 MockSettings() {
  list->setObjectName("listWidget"); pages->setObjectName("settingsPages"); box->setObjectName("buttonBox");
  list->addItem("General"); pages->addWidget(new QWidget);
  connect(box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(on_buttonBox_clicked(QAbstractButton*)));
 }
public slots:
 void on_buttonBox_clicked(QAbstractButton *button) {
  ++nativeCalls;
  if (box->buttonRole(button) == QDialogButtonBox::ApplyRole && !rejectApply) box->button(QDialogButtonBox::Apply)->setEnabled(false);
 }
};

int main(int argc, char **argv) {
 QApplication app(argc, argv);
 QTemporaryDir root;
 QMainWindow main;
 auto *controls = new QWidget(&main);
 auto *layout = new QVBoxLayout(controls); layout->setObjectName("buttonsVLayout");
 auto *youtube = new QPushButton("YouTube"); layout->addWidget(youtube);
 auto *settings = new QAction(&main); settings->setObjectName("action_Settings");
 hhc::KeyController controller(root.path());
 auto ui = std::make_unique<hhc::NativeUi>(&main, controller);
 int failed = !ui->install();
 auto check = [&](bool failure, int line) { if (failure) { ++failed; std::cerr << "FAIL line " << line << "\n"; } };
 check(layout->itemAt(0)->widget() != youtube || layout->itemAt(1)->widget() != controller.view(), __LINE__);
 auto closeMain = [&](QMessageBox::StandardButton answer, bool expectPrompt, bool expectClose) {
  bool prompted = false;
  QTimer::singleShot(0, [&] {
   auto *prompt = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
   if (!prompt) return;
   prompted = true;
   check(prompt->defaultButton() != prompt->button(QMessageBox::Cancel), __LINE__);
   QCloseEvent duplicate;
   QApplication::sendEvent(&main, &duplicate);
   check(duplicate.isAccepted(), __LINE__);
   prompt->button(answer)->click();
  });
  QCloseEvent close;
  QApplication::sendEvent(&main, &close);
  QApplication::processEvents();
  check(prompted != expectPrompt || close.isAccepted() != expectClose, __LINE__);
 };
 closeMain(QMessageBox::Cancel, false, true);
 hhc::KeyControllerTest::setFinalizing(controller, false);
 closeMain(QMessageBox::Cancel, true, false);
 closeMain(QMessageBox::Yes, true, true);
 hhc::KeyControllerTest::setFinalizing(controller, true);
 closeMain(QMessageBox::Cancel, false, true);
 for (int round = 0; round < 2; ++round) {
  auto *dialog = new MockSettings;
  check(!ui->attach(dialog) || !ui->attach(dialog) || dialog->pages->count() != 2, __LINE__);
  dialog->list->setCurrentRow(1);
  check(dialog->pages->currentWidget() != controller.settingsPage() || controller.settingsPage()->isWindow(), __LINE__);
  controller.keyField()->setText("invalid-test-key");
  controller.keyField()->setModified(true);
  dialog->box->button(QDialogButtonBox::Apply)->click();
  check(dialog->nativeCalls != 0, __LINE__); // Invalid HHC data cannot close or apply native settings.
  controller.keyField()->setText("hhc_stream_" + QString(43, 'A'));
  controller.keyField()->setModified(true);
  dialog->rejectApply = true;
  dialog->box->button(QDialogButtonBox::Ok)->click();
  check(dialog->nativeCalls != 1 || !controller.keyField()->isModified(), __LINE__); // Native rejection must not save credentials.
  auto answerNextPrompt = [](QMessageBox::StandardButton answer) {
   QTimer::singleShot(0, [answer] {
    for (auto *widget : QApplication::topLevelWidgets())
     if (auto *prompt = qobject_cast<QMessageBox *>(widget)) prompt->button(answer)->click();
   });
  };
  QCloseEvent cancelClose;
  answerNextPrompt(QMessageBox::Cancel);
  QApplication::sendEvent(dialog, &cancelClose);
  check(cancelClose.isAccepted() || !controller.keyField()->isModified(), __LINE__);
  QCloseEvent rejectedSave;
  answerNextPrompt(QMessageBox::Save);
  QApplication::sendEvent(dialog, &rejectedSave);
  check(rejectedSave.isAccepted() || !controller.keyField()->isModified(), __LINE__);
  QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  answerNextPrompt(QMessageBox::Discard);
  QApplication::sendEvent(dialog, &escape);
  check(controller.keyField()->isModified() || !controller.keyField()->text().isEmpty(), __LINE__);
  dialog->nativeCalls = 0;
  dialog->box->button(QDialogButtonBox::Cancel)->click();
  check(dialog->nativeCalls != 1 || !controller.keyField()->text().isEmpty(), __LINE__);
  dialog->box->button(QDialogButtonBox::Ok)->click();
  check(dialog->nativeCalls != 2, __LINE__); // Empty HHC field preserves key and forwards original settings.
  auto *autoRecord = controller.settingsPage()->findChild<QCheckBox *>("RecordWhenStreaming");
  dialog->box->button(QDialogButtonBox::Apply)->setEnabled(false);
  autoRecord->setChecked(true);
  check(!controller.settingsModified() || !dialog->box->button(QDialogButtonBox::Apply)->isEnabled(), __LINE__);
  dialog->rejectApply = true;
  dialog->box->button(QDialogButtonBox::Apply)->click();
  check(!controller.settingsModified(), __LINE__);
  dialog->rejectApply = false;
  dialog->box->button(QDialogButtonBox::Apply)->click();
  check(controller.settingsModified(), __LINE__);
  autoRecord->setChecked(false);
  answerNextPrompt(QMessageBox::Discard);
  QKeyEvent discardOptions(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  QApplication::sendEvent(dialog, &discardOptions);
  check(!autoRecord->isChecked() || controller.settingsModified(), __LINE__);
  autoRecord->setChecked(false);
  dialog->box->button(QDialogButtonBox::Apply)->click();
  delete dialog;
  check(controller.settingsPage()->parentWidget() != controller.view(), __LINE__);
 }
 hhc::KeyControllerTest::setFinalizing(controller, false);
 QTimer::singleShot(0, [&] {
  if (auto *prompt = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
   ui.reset();
   prompt->reject();
  }
 });
 QCloseEvent interruptedClose;
 QApplication::sendEvent(&main, &interruptedClose);
 QApplication::processEvents();
 check(ui || interruptedClose.isAccepted(), __LINE__);
 hhc::KeyControllerTest::setFinalizing(controller, true);
 if (failed) std::cerr << "Native UI integration failures: " << failed << '\n';
 return failed ? 1 : 0;
}
#include "native-ui-test.moc"
