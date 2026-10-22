#include "LaunchCommand.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

static bool regularFile(const std::string& path,size_t minimum=1) {
    struct stat info{};
    return stat(path.c_str(),&info)==0 && S_ISREG(info.st_mode) && info.st_size>=static_cast<off_t>(minimum);
}
bool validateXboxLaunchFiles(const XboxLaunchCommand& command,std::string& error) {
    error.clear();
    if(!regularFile(command.executable,24)) {
        error="THE EMULATOR EXECUTABLE IS MISSING OR UNREADABLE."; return false;
    }
    FILE* executable=fopen(command.executable.c_str(),"rb");
    char magic[4]{};
    const bool xex=executable && fread(magic,1,sizeof magic,executable)==sizeof magic && !memcmp(magic,"XEX2",4);
    if(executable) fclose(executable);
    if(!xex) { error="THE SELECTED EMULATOR IS NOT AN XBOX EXECUTABLE."; return false; }
    if(!command.rom.empty()) {
        std::string path=command.rom;
        std::replace(path.begin(),path.end(),'\\','/');
        const size_t colon=path.find(':');
        if(colon==std::string::npos) { error="INVALID GAME FILE PATH."; return false; }
        std::transform(path.begin(),path.begin()+colon,path.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(path.rfind("usb0:",0)==0) path.replace(0,5,"usb:");
        if(!regularFile(path)) { error="THE GAME FILE IS MISSING OR EMPTY."; return false; }
    }
    return true;
}
