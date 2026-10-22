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
int runSystemCommand(const std::string& text) {
    XboxLaunchCommand command; std::string error;
    if(!parseXboxLaunchCommand(text,command,error)) { LOG(LogError)<<error; errno=EINVAL; return -1; }
    if(!Settings::getInstance()->getBool("XboxLaunchEnabled")) {
        LOG(LogWarning)<<"Xbox title launching is disabled during frontend bring-up";
        errno=EPERM; return -1;
    }
    if(!command.rom.empty()) {
        std::string path=command.rom;
        if(path.rfind("Hdd:",0)==0) path.replace(0,4,"hdd:");
        if(path.rfind("Usb0:",0)==0) path.replace(0,5,"usb:");
        FILE* file=fopen(path.c_str(),"rb");
        if(!file) { LOG(LogError)<<"ROM file unavailable"; return -1; }
        fclose(file);
    }
    const void* data=command.rom.empty() ? nullptr : command.rom.c_str();
    size_t bytes=command.rom.empty() ? 0 : command.rom.size()+1;
    Log::flush();
    return xsdk_launch(command.executable.c_str(),data,bytes);
}
