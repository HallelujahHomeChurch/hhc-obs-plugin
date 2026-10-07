#include <obs-module.h>
#include "capture-output.hpp"
OBS_DECLARE_MODULE()
MODULE_EXPORT const char* obs_module_description(void) { return "HHC independent Windows capture"; }
#ifdef HHC_FIXTURE_BUILD
void registerFixtureController();
#endif
bool obs_module_load(void) {
 hhc::CaptureOutput::registerOutput();
#ifdef HHC_FIXTURE_BUILD
 registerFixtureController();
#endif
 return true;
}
