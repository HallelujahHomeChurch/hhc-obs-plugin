#include <obs-module.h>
#include "capture-output.hpp"
#include <QtGlobal>
#ifndef HHC_FIXTURE_BUILD
#include "local-controller.hpp"
#include <obs-frontend-api.h>
#include <util/bmem.h>
#include <QMainWindow>
namespace {
std::unique_ptr<hhc::LocalController> controller;
void frontendEvent(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && !controller) {
		char *path = obs_module_config_path("local-test-queue");
		const auto root = QString::fromUtf8(path);
		bfree(path);
		controller = std::make_unique<hhc::LocalController>(
			root, static_cast<QMainWindow *>(obs_frontend_get_main_window()));
		if (!obs_frontend_add_dock_by_id("hhc.capture", "HHC 影音", controller->view()))
			controller.reset();
		else
			blog(LOG_INFO, "[HHC] Local validation dock ready; platform connection unavailable");
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
#ifdef HHC_FIXTURE_BUILD
	if (!qEnvironmentVariableIsSet("HHC_FIXTURE_NATIVE_SMOKE"))
#endif
		hhc::CaptureOutput::registerOutput();
#ifdef HHC_FIXTURE_BUILD
	registerFixtureController();
#else
	obs_frontend_add_event_callback(frontendEvent, nullptr);
#endif
	return true;
}
