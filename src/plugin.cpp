#include <obs-module.h>
#include "capture-output.hpp"
#include <QtGlobal>
#ifndef HHC_FIXTURE_BUILD
#include "key-controller.hpp"
#include "native-ui.hpp"
#include <obs-frontend-api.h>
#include <util/bmem.h>
#include <util/config-file.h>
#include <QMainWindow>
#include <QCoreApplication>
namespace {
std::unique_ptr<hhc::KeyController> controller;
std::unique_ptr<hhc::NativeUi> nativeUi;
void frontendEvent(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && !controller) {
		char *tlsPath = obs_module_file("qt-plugins");
		if (tlsPath) {
			QCoreApplication::addLibraryPath(QString::fromUtf8(tlsPath));
			bfree(tlsPath);
		}
		char *path = obs_module_config_path("platform");
		const auto root = QString::fromUtf8(path);
		bfree(path);
		controller = std::make_unique<hhc::KeyController>(
			root, static_cast<QMainWindow *>(obs_frontend_get_main_window()), []() -> unsigned {
				auto *config = obs_frontend_get_profile_config();
				if (!config) return 0;
				const auto *mode = config_get_string(config, "Output", "Mode");
				if (!mode) return 0;
				return QString::fromUtf8(mode).compare("Advanced", Qt::CaseInsensitive) == 0
					? unsigned(config_get_uint(config, "AdvOut", "TrackIndex")) : 1u;
			});
		nativeUi = std::make_unique<hhc::NativeUi>(static_cast<QMainWindow *>(obs_frontend_get_main_window()), *controller);
		if (!nativeUi->install()) {
			blog(LOG_ERROR, "[HHC] OBS UI layout unsupported; HHC controls were not installed");
			nativeUi.reset(); controller.reset();
		} else blog(LOG_INFO, "[HHC] Native controls/status/settings integration ready");
	}
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		nativeUi.reset();
		controller.reset();
	}
}
} // namespace
#endif
OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_description(void)
{
	return "HHC independent Windows capture";
}
#ifdef HHC_FIXTURE_BUILD
void registerFixtureController();
#endif
bool obs_module_load(void)
{
#ifndef HHC_FIXTURE_BUILD
	blog(LOG_INFO, "[HHC] compiled source %s", HHC_SOURCE_COMMIT);
#ifdef HHC_NETWORK_TEST_FAULTS
	blog(LOG_WARNING, "[HHC] NETWORK FAULT TEST BUILD - not a distribution candidate");
#endif
#endif
#ifdef HHC_FIXTURE_BUILD
	if (!qEnvironmentVariableIsSet("HHC_FIXTURE_NATIVE_SMOKE") &&
	    !qEnvironmentVariableIsSet("HHC_FIXTURE_NATIVEDRIVE"))
#endif
		hhc::CaptureOutput::registerOutput();
#ifdef HHC_FIXTURE_BUILD
	registerFixtureController();
#else
	obs_frontend_add_event_callback(frontendEvent, nullptr);
#endif
	return true;
}
