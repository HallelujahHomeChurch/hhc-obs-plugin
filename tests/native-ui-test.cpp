#include "native-ui.hpp"
#include <QTemporaryDir>
#include <iostream>

class MockSettings : public QDialog {
 Q_OBJECT
public:
 QListWidget *list = new QListWidget(this);
 QStackedWidget *pages = new QStackedWidget(this);
 QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
 int nativeCalls = 0;
 MockSettings() {
  list->setObjectName("listWidget"); pages->setObjectName("settingsPages"); box->setObjectName("buttonBox");
  list->addItem("General"); pages->addWidget(new QWidget);
  connect(box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(on_buttonBox_clicked(QAbstractButton*)));
 }
public slots:
 void on_buttonBox_clicked(QAbstractButton *) { ++nativeCalls; }
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
 hhc::NativeUi ui(&main, controller);
 int failed = !ui.install();
 failed += layout->itemAt(0)->widget() != youtube || layout->itemAt(1)->widget() != controller.view();
 for (int round = 0; round < 2; ++round) {
  auto *dialog = new MockSettings;
  failed += !ui.attach(dialog) || !ui.attach(dialog) || dialog->pages->count() != 2;
  dialog->list->setCurrentRow(1);
  failed += dialog->pages->currentWidget() != controller.settingsPage() || controller.settingsPage()->isWindow();
  controller.keyField()->setText("invalid-test-key");
  controller.keyField()->setModified(true);
  dialog->box->button(QDialogButtonBox::Apply)->click();
  failed += dialog->nativeCalls != 0; // Invalid HHC data cannot close or apply native settings.
  dialog->box->button(QDialogButtonBox::Cancel)->click();
  failed += dialog->nativeCalls != 1 || !controller.keyField()->text().isEmpty();
  dialog->box->button(QDialogButtonBox::Ok)->click();
  failed += dialog->nativeCalls != 2; // Empty HHC field preserves key and forwards original settings.
  delete dialog;
  failed += controller.settingsPage()->parentWidget() != controller.view();
 }
 if (failed) std::cerr << "Native settings integration failures: " << failed << '\n';
 return failed ? 1 : 0;
}
#include "native-ui-test.moc"
