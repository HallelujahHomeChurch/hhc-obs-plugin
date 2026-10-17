#pragma once
#include "key-controller.hpp"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QStatusBar>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QEvent>
#include <QAction>
namespace hhc {
// shortcut: OBS 32.2.2 widget names are internal; revalidate before supporting another OBS version.
class NativeUi : public QObject {
public:
 NativeUi(QMainWindow *main, KeyController &controller) : QObject(main), main_(main), controller_(controller) {}
 ~NativeUi() override {
  qApp->removeEventFilter(this);
  controller_.openSettings = {};
  if (dialog_) {
   controller_.settingsPage()->hide();
   controller_.settingsPage()->setParent(controller_.view());
  }
  if (controller_.statusLabel()) controller_.statusLabel()->setParent(controller_.view());
 }
 bool install() {
  auto *buttons = main_->findChild<QVBoxLayout *>("buttonsVLayout");
  auto *settingsAction = main_->findChild<QAction *>("action_Settings");
  if (!buttons || !settingsAction || !main_->statusBar()) return false;
  controller_.view()->layout()->setContentsMargins(0, 0, 0, 0);
  buttons->insertWidget(1, controller_.view());
  main_->statusBar()->addPermanentWidget(controller_.statusLabel());
  controller_.view()->show();
  controller_.statusLabel()->show();
  controller_.openSettings = [this, settingsAction] { selectHhc_ = true; settingsAction->trigger(); };
  qApp->installEventFilter(this);
  return true;
 }
 bool attach(QDialog *dialog) {
  if (dialog->property("hhcAttached").toBool()) return true;
  auto *list = dialog->findChild<QListWidget *>("listWidget");
  auto *pages = dialog->findChild<QStackedWidget *>("settingsPages");
  auto *box = dialog->findChild<QDialogButtonBox *>("buttonBox");
  if (!list || !pages || !box || list->count() == 0 || pages->count() != list->count() ||
      dialog->metaObject()->indexOfMethod("on_buttonBox_clicked(QAbstractButton*)") < 0) return false;
  // Preserve the original handler, but validate HHC before it can close the dialog.
  if (!QObject::disconnect(box, SIGNAL(clicked(QAbstractButton*)), dialog, SLOT(on_buttonBox_clicked(QAbstractButton*)))) return false;
  dialog_ = dialog;
  dialog->setProperty("hhcAttached", true);
  auto *page = controller_.settingsPage();
  controller_.beginSettings();
  const int index = pages->addWidget(page);
  auto *item = new QListWidgetItem(QString::fromUtf8("HHC"), list);
  item->setIcon(list->item(0)->icon());
  connect(list, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
  connect(controller_.keyField(), &QLineEdit::textEdited, dialog, [box](const QString &) {
   if (auto *apply = box->button(QDialogButtonBox::Apply)) apply->setEnabled(true);
  });
  connect(box, &QDialogButtonBox::clicked, this, [this, dialog, box, list, index](QAbstractButton *button) {
   auto role = box->buttonRole(button);
   if ((role == QDialogButtonBox::ApplyRole || role == QDialogButtonBox::AcceptRole) && !controller_.applySettings()) {
    list->setCurrentRow(index);
    return;
   }
   if (role == QDialogButtonBox::RejectRole) controller_.discardSettings();
   QMetaObject::invokeMethod(dialog, "on_buttonBox_clicked", Qt::DirectConnection, Q_ARG(QAbstractButton*, button));
  });
  connect(dialog, &QObject::destroyed, this, [this] {
   controller_.discardSettings();
   controller_.settingsPage()->hide();
   controller_.settingsPage()->setParent(controller_.view());
   dialog_ = nullptr;
  });
  if (selectHhc_) list->setCurrentRow(index);
  selectHhc_ = false;
  return true;
 }
protected:
 bool eventFilter(QObject *object, QEvent *event) override {
  if (event->type() == QEvent::Show && QByteArray(object->metaObject()->className()) == "OBSBasicSettings") {
   if (auto *dialog = qobject_cast<QDialog *>(object)) attach(dialog);
  }
  return false;
 }
private:
 QMainWindow *main_;
 KeyController &controller_;
 QPointer<QDialog> dialog_;
 bool selectHhc_ = false;
};
}
