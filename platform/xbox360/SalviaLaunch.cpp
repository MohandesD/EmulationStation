#include "SalviaLaunch.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <sys/stat.h>

namespace {
using Bytes=std::vector<unsigned char>;
constexpr size_t MenuBytes=8496;
// Salvia v1.0.10 src/beans/structures.h, Xbox 32-bit big-endian ListStatus.
constexpr size_t ZipName=41,ZipPath=301,RelativePath=4397,StringsEnd=8493;
struct Plan { std::string folder; Bytes before,after; };

bool read(const std::string& path,Bytes& data,size_t maximum) {
    struct stat st{};
    if(stat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0 ||
       static_cast<uint64_t>(st.st_size)>maximum) return false;
    FILE* file=std::fopen(path.c_str(),"rb"); if(!file) return false;
    data.resize(static_cast<size_t>(st.st_size));
    const bool ok=std::fread(data.data(),1,data.size(),file)==data.size() && !std::ferror(file);
    const int closed=std::fclose(file);
    return ok && closed==0;
}
std::string trim(const std::string& s) {
    const auto first=s.find_first_not_of(" \t\r\n");
    return first==std::string::npos ? "" : s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}
std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});
    return s;
}
bool config(const std::string& path,std::map<std::string,std::string>& values) {
    Bytes data; if(!read(path,data,65536) || std::find(data.begin(),data.end(),0)!=data.end()) return false;
    std::istringstream stream(std::string(data.begin(),data.end())); std::string line;
    while(std::getline(stream,line)) {
        line=trim(line); if(line.empty() || line[0]=='#') continue;
        const auto eq=line.find('='); if(eq==std::string::npos) continue;
        const auto key=trim(line.substr(0,eq));
        if(!values.emplace(key,trim(line.substr(eq+1))).second) return false;
    }
    return true;
}
uint32_t be32(const Bytes& b,size_t i) {
    return (uint32_t(b[i])<<24)|(uint32_t(b[i+1])<<16)|(uint32_t(b[i+2])<<8)|b[i+3];
}
void set32(Bytes& b,size_t i,uint32_t value) {
    b[i]=value>>24; b[i+1]=value>>16; b[i+2]=value>>8; b[i+3]=value;
}
bool terminated(const Bytes& b,size_t begin,size_t end) {
    return std::find(b.begin()+begin,b.begin()+end,0)!=b.begin()+end;
}
bool systemName(const std::string& name) {
    return !name.empty() && name.size()<=38 &&
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")==std::string::npos;
}
bool readableConfig(const std::string& path) {
    // Salvia creates a positional entry once a regular configuration file
    // opens, even when that file is empty. Its contents need not be parsed to
    // establish the index of a later system.
    struct stat st{};
    if(stat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode)) return false;
    FILE* file=std::fopen(path.c_str(),"rb");
    return file && std::fclose(file)==0;
}
bool plan(const XboxLaunchCommand& cmd,Plan& p,std::string& error) {
    p={}; error.clear();
    if(cmd.salviaSystem.empty()) {
        const auto slash=cmd.executable.find_last_of('/');
        struct stat st{};
        if(!cmd.rom.empty() && slash!=std::string::npos &&
           stat((cmd.executable.substr(0,slash+1)+"salvia.cfg").c_str(),&st)==0) {
            error="THIS SALVIA COMMAND MUST SPECIFY ITS SYSTEM. UPDATE THE GAME CONFIGURATION.";
            return false;
        }
        return true;
    }
    error="SALVIA'S SYSTEM CONFIGURATION COULD NOT BE VERIFIED.";
    if(cmd.rom.empty() || !systemName(cmd.salviaSystem)) return false;
    const auto slash=cmd.executable.find_last_of('/'); if(slash==std::string::npos) return false;
    p.folder=cmd.executable.substr(0,slash+1);
    std::map<std::string,std::string> main,system;
    if(!config(p.folder+"salvia.cfg",main) || !config(p.folder+"config/"+cmd.salviaSystem+".cfg",system)) return false;
    std::istringstream names(main["emulators"]); std::vector<std::string> systems;
    std::set<std::string> unique; std::string name;
    // Match Constant::splitChar(emulators, ' '), not stream whitespace
    // extraction: tabs belong to names and repeated spaces create empty
    // names upstream. Reject those forms rather than inventing their indices.
    while(std::getline(names,name,' ')) {
        if(systems.size()>=128 || !systemName(name) || !unique.insert(name).second) return false;
        systems.push_back(name);
    }
    const auto selected=std::find(systems.begin(),systems.end(),cmd.salviaSystem);
    if(selected==systems.end() || !system["directory"].empty()) return false;
    // CfgLoader::loadEmuConfig skips files it cannot open. Counting a missing
    // predecessor would select another system (or Options) in menu.tmp and
    // can apply the wrong ROM-loading settings. Do not silently remap it.
    for(auto previous=systems.begin();previous!=selected;++previous) {
        if(!readableConfig(p.folder+"config/"+*previous+".cfg")) {
            error="AN EMULATOR SETUP FILE IS MISSING OR UNREADABLE. THE GAME WAS NOT STARTED.";
            return false;
        }
    }
    std::istringstream cores(system["executable"]); bool found=false;
    while(std::getline(cores,name,';')) if(lower(trim(name))==lower(cmd.executable.substr(slash+1))) found=true;
    if(!found || (system["no_uncompress"]!="yes" && system["no_uncompress"]!="no")) return false;
    error="SALVIA'S SAVED MENU HAS AN UNRECOGNIZED FORMAT. LAUNCH WAS CANCELLED.";
    if(!read(p.folder+"menu.tmp",p.before,MenuBytes) || p.before.size()!=MenuBytes ||
       be32(p.before,0)>systems.size() || p.before[24]>1 || p.before[40]>1 ||
       !terminated(p.before,ZipName,ZipPath) || !terminated(p.before,ZipPath,RelativePath) ||
       !terminated(p.before,RelativePath,StringsEnd)) return false;
    p.after=p.before;
    const auto index=static_cast<uint32_t>(selected-systems.begin());
    if(be32(p.before,0)!=index || p.before[ZipName] || p.before[ZipPath] || p.before[RelativePath]) {
        set32(p.after,0,index);
        // Force Salvia to reconstruct the new system's list; keep layout/background.
        std::fill(p.after.begin()+4,p.after.begin()+20,0);
        std::fill(p.after.begin()+28,p.after.begin()+StringsEnd,0);
    }
    error.clear(); return true;
}
bool writeVerified(const std::string& path,const Bytes& data) {
    FILE* file=std::fopen(path.c_str(),"wb"); if(!file) return false;
    const bool ok=std::fwrite(data.data(),1,data.size(),file)==data.size();
    const int closed=std::fclose(file);
    Bytes actual; return ok && closed==0 && read(path,actual,MenuBytes) && actual==data;
}
bool replace(const std::string& path,const Bytes& data) {
    const std::string temp=path+".tmp";
    if(!writeVerified(temp,data) || std::rename(temp.c_str(),path.c_str())!=0) return false;
    Bytes actual; return read(path,actual,MenuBytes) && actual==data;
}
}
bool validateSalviaLaunch(const XboxLaunchCommand& cmd,std::string& error,bool requireReady) {
    Plan p; if(!plan(cmd,p,error)) return false;
    if(requireReady && p.before!=p.after) {
        error="SALVIA'S SYSTEM SELECTION HAS NOT BEEN PREPARED."; return false;
    }
    return true;
}
bool prepareSalviaLaunch(const XboxLaunchCommand& cmd,std::string& error) {
    Plan p; if(!plan(cmd,p,error)) return false;
    if(p.before==p.after) return true;
    error="COULD NOT SAVE SALVIA'S SYSTEM SELECTION. LAUNCH WAS CANCELLED.";
    // Preserve the immediate prior state, then verify it has not changed meanwhile.
    if(!replace(p.folder+"menu.es-backup",p.before)) return false;
    Bytes actual;
    if(!read(p.folder+"menu.tmp",actual,MenuBytes) || actual!=p.before) return false;
    if(!replace(p.folder+"menu.tmp",p.after)) {
        // The verified backup remains available even if rollback cannot be written.
        replace(p.folder+"menu.tmp",p.before);
        return false;
    }
    error.clear(); return true;
}
