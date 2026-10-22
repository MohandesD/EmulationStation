// Exercise the production native orchestration with a returning mock loader:
// the real loader replaces the process on success and cannot be used on a PC.
#include "LaunchCommand.h"
#include "SalviaLaunch.h"
#include "SessionReturn.h"
#include "Settings.h"
#include "Log.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {
bool enabled=true,filesValid=true,contextValid=true,prepared=true,restored=true;
int prepares=0,restores=0,launches=0,loaderResult=-1;
const std::string recovery="COULD NOT RESTORE THE PREVIOUS DASHBOARD SETTINGS. STAY IN EMULATIONSTATION.";
const std::string command="salvia nes hdd:/Salvia/Nestopia.xex hdd:/ROMs/nes/Tetris.nes";
void reset() {
    enabled=filesValid=contextValid=prepared=restored=true;
    prepares=restores=launches=0;loaderResult=-1;
}
}

Settings::Settings() = default;
Settings* Settings::getInstance() { static Settings instance;return &instance; }
bool Settings::getBool(const std::string&) { return enabled; }
const std::string& Settings::getString(const std::string&) { static const std::string empty;return empty; }
LogLevel Log::getReportingLevel() { return LogError; }
std::ostringstream& Log::get(LogLevel) { return os; }
Log::~Log() = default;
void Log::flush() {}

bool validateXboxLaunchFiles(const XboxLaunchCommand&,std::string& error) {
    if(!filesValid) error="THE EMULATOR FILE IS MISSING.";
    return filesValid;
}
bool validateSalviaLaunch(const XboxLaunchCommand&,std::string& error,bool ready) {
    assert(ready);
    if(!contextValid) error="SALVIA CONTEXT IS NOT READY.";
    return contextValid;
}
bool prepareXboxSessionReturn(std::string& error) {
    prepares++;
    if(!prepared) error="COULD NOT PREPARE RETURN TO EMULATIONSTATION. THE GAME WAS NOT STARTED.";
    return prepared;
}
bool restoreXboxSessionReturn(std::string& error) {
    restores++;
    if(!restored) error=recovery;
    return restored;
}
int xsdk_launch(const char* path,const void* data,size_t size) {
    launches++;
    assert(std::strcmp(path,"hdd:/Salvia/Nestopia.xex")==0);
    const char* rom="Hdd:\\ROMs\\nes\\Tetris.nes";
    assert(size==std::strlen(rom)+1 && std::strcmp(static_cast<const char*>(data),rom)==0);
    return loaderResult;
}

int main() {
    std::string error="stale previous error";
    assert(runXboxSystemCommand(command,error)==-1);
    assert(prepares==1 && launches==1 && restores==1);
    assert(error=="THE EMULATOR COULD NOT BE STARTED. THE GAME LIST HAS BEEN RESTORED.");

    // The actionable recovery error must reach SystemData, not just a log. A
    // nominal zero from a returned loader still cannot hide failed restoration.
    reset();restored=false;loaderResult=0;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(prepares==1 && launches==1 && restores==1);
    assert(error.find(recovery)!=std::string::npos);
    assert(error.find("THE GAME LIST HAS BEEN RESTORED")==std::string::npos);

    reset();filesValid=false;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(prepares==1 && launches==0 && restores==1);
    assert(error=="THE EMULATOR FILE IS MISSING.");

    reset();filesValid=false;restored=false;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(launches==0 && restores==1 && error.find(recovery)!=std::string::npos);

    reset();contextValid=false;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(launches==0 && restores==1 && error=="SALVIA CONTEXT IS NOT READY.");

    reset();prepared=false;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(prepares==1 && launches==0 && restores==0 && error.find("GAME WAS NOT STARTED")!=std::string::npos);

    reset();enabled=false;
    assert(runXboxSystemCommand(command,error)==-1);
    assert(prepares==0 && launches==0 && restores==0 && error.find("NOT BEEN ENABLED")!=std::string::npos);

    reset();
    assert(runXboxSystemCommand("not a launch command",error)==-1);
    assert(prepares==0 && launches==0 && restores==0 && !error.empty());
    std::cout<<"Native handoff failure, recovery-error propagation and gate checks passed\n";
}
