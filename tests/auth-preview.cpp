#include "native-auth.hpp"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (argc < 2)
		return 2;
	hhc::NativeAuth auth(QString::fromLocal8Bit(argv[1]));
	QWidget window;
	window.setWindowTitle(QString::fromUtf8("HHC OBS — 正式平台登入測試"));
	auto *layout = new QVBoxLayout(&window);
	auto *label = new QLabel(QString::fromUtf8("登入 HHC 正式平台；密碼僅在系統瀏覽器輸入。"));
	label->setWordWrap(true);
	layout->addWidget(label);
	auto *button = new QPushButton(QString::fromUtf8("登入 HHC"));
	layout->addWidget(button);
	QObject::connect(button, &QPushButton::clicked, [&] {
		try {
			auth.login();
			label->setText(QString::fromUtf8("請在系統瀏覽器完成登入與授權。"));
		} catch (...) {
			label->setText(QString::fromUtf8("無法啟動登入，請重試。"));
		}
	});
	auth.onChanged = [&](QString error) {
		label->setText(error.isEmpty()
				       ? QString::fromUtf8("登入成功 · 帳號 %1\n錄影 %2 · 發布 %3")
						 .arg(auth.account())
						 .arg(auth.permitted("cms:recordings:write") ? "OK" : "DENIED")
						 .arg(auth.permitted("cms:recordings:publish") ? "OK" : "DENIED")
				       : error);
	};
	window.resize(480, 180);
	window.show();
	if (argc == 3)
		QTimer::singleShot(500, button, &QPushButton::click);
	return app.exec();
}
