#include "LaunchCommand.h"
#include <algorithm>
#include <cctype>
#include <vector>

static bool normalize(std::string& path,bool rom) {
    if(path.size()>900 || path.find_first_of("\r\n\t\"<>|?*")!=std::string::npos) return false;
    std::replace(path.begin(),path.end(),'\\','/');
    size_t colon=path.find(':');
    if(colon==std::string::npos || colon+1>=path.size() || path[colon+1]!='/') return false;
    std::string drive=path.substr(0,colon);
    std::transform(drive.begin(),drive.end(),drive.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(drive=="hdd1") drive="hdd";
    if(drive=="usb0") drive="usb";
    if(drive!="hdd" && drive!="usb" && drive!="usb1" && drive!="usb2" && drive!="dvd" && drive!="game") return false;
    const std::string tail=path.substr(colon+1);
    if(tail.find(':')!=std::string::npos) return false;
    for(size_t start=1;start<tail.size();) {
        size_t end=tail.find('/',start); if(end==std::string::npos) end=tail.size();
        std::string part=tail.substr(start,end-start);
        if(part.empty() || part=="." || part==".." || part.size()>42) return false;
        start=end+1;
    }
    if(tail.back()=='/') return false;
    // Salvia v1.0.10 Launcher::initDrives mounts Hdd: and Usb0:/Usb1:/Usb2:.
    if(rom) {
        // game: changes on handoff; Salvia does not mount Dvd: for ROMs.
        if(drive=="game" || drive=="dvd") return false;
        if(drive=="hdd") drive="Hdd";
        else if(drive=="usb") drive="Usb0";
        else if(drive=="usb1") drive="Usb1";
        else if(drive=="usb2") drive="Usb2";
    }
    path=drive+":"+tail;
    if(rom) std::replace(path.begin(),path.end(),'/','\\');
    return true;
}
bool parseXboxLaunchCommand(const std::string& text,XboxLaunchCommand& out,std::string& error) {
    out={}; error.clear(); std::vector<std::string> tokens;
    for(size_t i=0;i<text.size();) {
        if(text[i]==' ') { ++i; continue; }
        bool quoted=text[i]=='"'; if(quoted) ++i;
        std::string token;
        while(i<text.size() && (quoted ? text[i]!='"' : text[i]!=' ')) {
            unsigned char c=text[i++];
            if(c<32 || c==127 || (!quoted && c=='"')) { error="Invalid command character"; return false; }
            token+=char(c);
        }
        if(quoted) {
            if(i==text.size()) { error="Unclosed path quote"; return false; }
            ++i;
            if(i<text.size() && text[i]!=' ') { error="Expected space after path"; return false; }
        }
        if(token.empty()) { error="Empty command argument"; return false; }
        tokens.push_back(token);
        if(tokens.size()>3) { error="Expected executable and optional ROM only"; return false; }
    }
    if(tokens.size()<2 || tokens[0]!="xex") { error="Xbox commands must start with xex"; return false; }
    std::string executable=tokens[1];
    if(!normalize(executable,false)) { error="Invalid executable path"; return false; }
    std::string suffix=executable.size()>=4 ? executable.substr(executable.size()-4) : "";
    std::transform(suffix.begin(),suffix.end(),suffix.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(suffix!=".xex") { error="Executable must be an XEX"; return false; }
    std::string rom=tokens.size()==3 ? tokens[2] : "";
    if(!rom.empty() && !normalize(rom,true)) { error="Invalid ROM path"; return false; }
    out.executable=executable; out.rom=rom; return true;
}
