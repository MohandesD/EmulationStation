#include "platform.h"
#include "LaunchCommand.h"
#include "Settings.h"
#include "Log.h"
#include <xsdk/xam.h>
#include <cerrno>
#include <cstdio>

std::string getHomePath() { return "game:"; }
// Console shutdown/restart are not shell commands. The Xbox menu hides these.
int runShutdownCommand() { errno=ENOTSUP; return -1; }
int runRestartCommand() { errno=ENOTSUP; return -1; }
static int handoff(const XboxLaunchCommand& command) {
    std::string error;
    if(!validateXboxLaunchFiles(command,error)) { LOG(LogError)<<error; return -1; }
    const void* data=command.rom.empty() ? nullptr : command.rom.c_str();
    size_t bytes=command.rom.empty() ? 0 : command.rom.size()+1;
    Log::flush();
    return xsdk_launch(command.executable.c_str(),data,bytes);
}
int runSystemCommand(const std::string& text) {
    XboxLaunchCommand command; std::string error;
    if(!parseXboxLaunchCommand(text,command,error)) { LOG(LogError)<<error; errno=EINVAL; return -1; }
    if(!isXboxLaunchPermitted(command,Settings::getInstance()->getBool("XboxLaunchEnabled"),
        Settings::getInstance()->getString("XboxTestRom"))) {
        LOG(LogWarning)<<"Xbox title launching is disabled during frontend bring-up";
        errno=EPERM; return -1;
    }
    return handoff(command);
}
int runXboxHandoffDiagnostic() {
    XboxLaunchCommand command; std::string error;
    const char* text="xex hdd:/Apps/ESProbe/ESHandoffReceiver.xex hdd:/Apps/ESProbe/handoff-fixture.txt";
    if(!parseXboxLaunchCommand(text,command,error)) return -1;
    LOG(LogInfo)<<"Launching the fixed handoff diagnostic receiver";
    return handoff(command);
}
