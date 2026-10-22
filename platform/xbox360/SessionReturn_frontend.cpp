#include "SessionReturn.h"
#include "Settings.h"
#include <sys/stat.h>

bool prepareXboxSessionReturn(std::string& error) {
    error.clear();
    if(!Settings::getInstance()->getBool("XboxSessionReturn")) return true;
    auto runtime=openXboxReturnRuntime();
    if(!runtime) { error="SESSION RETURN IS UNAVAILABLE. THE GAME WAS NOT STARTED."; return false; }
    return XboxReturn::arm(*runtime,error);
}
bool restoreXboxSessionReturn(std::string& error) {
    error.clear(); struct stat st{};
    if(stat("hdd:/Apps/EmulationStation/SessionReturn/original.state",&st)!=0 &&
       !Settings::getInstance()->getBool("XboxSessionReturn")) return true;
    auto runtime=openXboxReturnRuntime();
    if(!runtime) { error="SESSION RETURN SETTINGS COULD NOT BE CHECKED."; return false; }
    return XboxReturn::restore(*runtime,error);
}
