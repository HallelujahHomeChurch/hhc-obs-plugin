#include "local-controller.hpp"
#include <QApplication>
#include <QAbstractButton>
#include <QCloseEvent>
#include <QElapsedTimer>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <iostream>
#include <cstdlib>
class VetoWindow : public QWidget {
public:
	int vetoes = 0;
	void closeEvent(QCloseEvent *event) override
	{
		++vetoes;
		event->ignore();
	}
};
template<class F> bool until(F check, int ms = 15000)
{
	QElapsedTimer timer;
	timer.start();
	while (!check() && timer.elapsed() < ms) {
		QApplication::processEvents();
		QThread::msleep(10);
	}
	return check();
}
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (argc == 3 && QString(argv[1]) == "--bad-root") {
		hhc::LocalController controller(QString::fromLocal8Bit(argv[2]));
		if (!until([&] {
			    return controller.view()
				    ->findChild<QLabel *>("recovery")
				    ->text()
				    .contains(QString::fromUtf8("無法"));
		    }))
			return 30;
		try {
			controller.shutdown();
		} catch (...) {
			std::cerr << "FAIL recovery exception escapes shutdown\n";
			std::cerr.flush();
			std::_Exit(31);
		}
		return 0;
	}
	if (!obs_startup("en-US", nullptr, nullptr))
		return 2;
	obs_add_data_path("C:/Program Files/obs-studio/data/libobs/");
	obs_video_info video{};
	video.graphics_module = "C:/Program Files/obs-studio/bin/64bit/libobs-d3d11.dll";
	video.base_width = video.output_width = 1920;
	video.base_height = video.output_height = 1080;
	video.fps_num = 30000;
	video.fps_den = 1001;
	video.output_format = VIDEO_FORMAT_NV12;
	video.colorspace = VIDEO_CS_709;
	video.range = VIDEO_RANGE_PARTIAL;
	video.gpu_conversion = true;
	if (obs_reset_video(&video) != OBS_VIDEO_SUCCESS)
		return 3;
	obs_audio_info audio{48000, SPEAKERS_STEREO};
	if (!obs_reset_audio(&audio))
		return 4;
	for (const char *name : {"obs-nvenc", "obs-ffmpeg"}) {
		obs_module_t *module = nullptr;
		const auto dll = QString("C:/Program Files/obs-studio/obs-plugins/64bit/%1.dll").arg(name).toUtf8();
		const auto data = QString("C:/Program Files/obs-studio/data/obs-plugins/%1").arg(name).toUtf8();
		if (obs_open_module(&module, dll.constData(), data.constData()) == MODULE_SUCCESS)
			obs_init_module(module);
	}
	obs_post_load_modules();
	hhc::CaptureOutput::registerOutput();
	QTemporaryDir root;
	int result = 0;
	{
		VetoWindow window;
		window.setAttribute(Qt::WA_DontShowOnScreen);
		window.show();
		hhc::LocalController controller(root.path(), &window);
		auto *action = controller.view()->findChild<QPushButton *>("action");
		if (!until([&] { return action->isEnabled(); }))
			return 5;
		action->click();
		if (!controller.busy())
			return 6;
		bool warmed = false;
		QTimer::singleShot(1500, [&] { warmed = true; });
		if (!until([&] { return warmed; }))
			return 9;
		QTimer::singleShot(0, [] {
			for (auto *widget : QApplication::topLevelWidgets())
				if (auto *box = qobject_cast<QMessageBox *>(widget))
					box->button(QMessageBox::Yes)->click();
		});
		const bool closed = window.close();
		if (window.vetoes != 1 || closed || !controller.busy()) {
			std::cerr << "FAIL OBS close veto disabled HHC capture (vetoes=" << window.vetoes << ")\n";
			result = 7;
		} else {
			action->click();
			if (!until([&] {
				    return !controller.busy() && controller.phase() == hhc::Phase::LocalComplete;
			    }))
				result = 8;
		}
	}
	obs_shutdown();
	return result;
}
