#include <obs-module.h>
#include "capture-output.hpp"
#include <QtGlobal>
#ifndef HHC_FIXTURE_BUILD
#include "platform-controller.hpp"
#include <obs-frontend-api.h>
#include <util/bmem.h>
#include <QMainWindow>
#include <QCoreApplication>
namespace {
std::unique_ptr<hhc::PlatformController> controller;
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
		controller = std::make_unique<hhc::PlatformController>(
			root, static_cast<QMainWindow *>(obs_frontend_get_main_window()));
		if (!obs_frontend_add_dock_by_id("hhc.capture", "HHC 影音", controller->view()))
			controller.reset();
		else
			blog(LOG_INFO,
			     "[HHC] Native platform dock ready; OAuth credentials stay in Credential Manager");
	}
	if (event == OBS_FRONTEND_EVENT_EXIT)
		controller.reset();
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
