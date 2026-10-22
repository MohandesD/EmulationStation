#include "SessionReturn.h"
#include "LaunchCommand.h"
#include <xsdk/kernel.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {
constexpr const char* Base="hdd:/Apps/EmulationStation/SessionReturn";
constexpr const char* Native="\\Device\\Harddisk0\\Partition1\\Apps\\EmulationStation\\SessionReturn\\";
constexpr const char* Home="hdd:/Apps/EmulationStation/emulationstation.xex";
struct KeyData { char path[260]; uint32_t flags,device,root; };
static_assert(sizeof(KeyData)==272,"DashLaunch path ABI");
using GetCount=int (*)(int*);
using GetInfo=int (*)(int,uint32_t*,char*,uint32_t*,uint32_t*,uint32_t*);
using GetDrive=uint32_t (*)(uint32_t,char*,char*,char*);
using Reload=void (*)(char*);

bool readFile(const std::string& path,std::string& out) {
    struct stat st{};
    if(stat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0 || st.st_size>131072) return false;
    FILE* f=std::fopen(path.c_str(),"rb"); if(!f) return false;
    out.resize(st.st_size); const bool ok=std::fread(out.data(),1,out.size(),f)==out.size() && !std::ferror(f);
    return std::fclose(f)==0 && ok;
}
bool writeFile(const std::string& path,const std::string& text) {
    FILE* f=std::fopen(path.c_str(),"wb"); if(!f) return false;
    const bool ok=std::fwrite(text.data(),1,text.size(),f)==text.size();
    const bool closed=std::fclose(f)==0; std::string actual;
    return ok && closed && readFile(path,actual) && actual==text;
}
bool directory(const std::string& path) {
    struct stat st{};
    if(stat(path.c_str(),&st)==0) return S_ISDIR(st.st_mode);
    return mkdir(path.c_str(),0777)==0;
}
struct Runtime final: XboxReturn::Backend {
    GetCount count=nullptr; GetInfo info=nullptr; GetDrive drive=nullptr; Reload reloadIni=nullptr;
    bool init() {
        HANDLE module=0;
        if(XexGetModuleHandle("launch.xex",&module)!=ERROR_SUCCESS) return false;
        void* entries[4]{}; const uint32_t ordinals[]={5,6,11,4};
        for(unsigned i=0;i<4;i++) if(!NT_SUCCESS(XexGetProcedureAddress(module,ordinals[i],&entries[i])) || !entries[i]) return false;
        count=reinterpret_cast<GetCount>(entries[0]); info=reinterpret_cast<GetInfo>(entries[1]);
        drive=reinterpret_cast<GetDrive>(entries[2]); reloadIni=reinterpret_cast<Reload>(entries[3]);
        return true;
    }
    bool read(XboxReturn::Snapshot& result) override {
        result.clear(); const int n=count(nullptr); if(n<=0 || n>256) return false;
        for(int i=0;i<n;i++) {
            XboxReturn::Option o; char name[64]{}; uint32_t raw=0;
            if(info(i,&o.type,name,&raw,nullptr,nullptr)!=0 || !std::memchr(name,0,sizeof name)) return false;
            o.name=name;
            if(o.type<6) o.value=raw;
            else {
                if(o.type>8 || raw<0x80000000u || raw>0xfffffef0u || (raw&3)) return false;
                const auto* key=reinterpret_cast<const KeyData*>(static_cast<uintptr_t>(raw));
                if(!std::memchr(key->path,0,sizeof key->path)) return false;
                o.path=key->path; o.flags=key->flags; o.device=key->device; o.root=key->root;
                if(o.root!=255) {
                    char prefix[260]{};
                    if(o.root>=64 || drive(o.root,nullptr,prefix,nullptr)<=o.root || !std::memchr(prefix,0,sizeof prefix)) return false;
                    o.prefix=prefix;
                }
            }
            result.push_back(o);
        }
        return XboxReturn::valid(result);
    }
    bool target(XboxReturn::Option& o) override {
        XboxLaunchCommand command; command.executable=Home; std::string error;
        if(!validateXboxLaunchFiles(command,error)) return false;
        const auto n=drive(0,nullptr,nullptr,nullptr); if(!n || n>64) return false;
        for(uint32_t i=0;i<n;i++) {
            char device[260]{},prefix[260]{};
            if(drive(i,device,prefix,nullptr)!=n || !std::memchr(device,0,sizeof device) || !std::memchr(prefix,0,sizeof prefix)) return false;
            if(std::strcmp(device,"\\Device\\Harddisk0\\Partition1")==0 || std::strcmp(device,"\\Device\\Harddisk0\\Partition1\\")==0) {
                o={}; o.name="Default"; o.type=7; o.prefix=prefix;
                o.path="\\Apps\\EmulationStation\\emulationstation.xex"; o.flags=1; o.device=i; o.root=i;
                return true;
            }
        }
        return false;
    }
    bool saveOriginal(const XboxReturn::Snapshot& s) override {
        const auto text=XboxReturn::encode(s); if(text.empty() || !directory(Base)) return false;
        const std::string path=std::string(Base)+"/original.state",temp=path+".tmp";
        if(!writeFile(temp,text) || std::rename(temp.c_str(),path.c_str())!=0) return false;
        std::string actual; return readFile(path,actual) && actual==text;
    }
    bool loadOriginal(XboxReturn::Snapshot& s) override {
        std::string text; return readFile(std::string(Base)+"/original.state",text) && XboxReturn::decode(text,s);
    }
    bool stage(const char* slot,const std::string& text) override {
        if(std::strcmp(slot,"active") && std::strcmp(slot,"restore")) return false;
        const std::string folder=std::string(Base)+"/"+slot;
        return !text.empty() && directory(Base) && directory(folder) && writeFile(folder+"/launch.ini",text);
    }
    void reload(const char* slot) override {
        if(std::strcmp(slot,"active") && std::strcmp(slot,"restore")) return;
        std::string path=std::string(Native)+slot; reloadIni(path.data());
    }
};
}
std::unique_ptr<XboxReturn::Backend> openXboxReturnRuntime() {
    auto runtime=std::make_unique<Runtime>();
    if(!runtime->init()) return {};
    return runtime;
}
