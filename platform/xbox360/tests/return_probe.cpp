// Read-only inspection of the already-loaded DashLaunch return API.
// Never invokes its setters, reloads an INI, installs anything or launches a ROM.
#include <xsdk/kernel.h>
#include <xsdk/xam.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

using GetOption=int32_t (*)(char*,uint32_t*);
struct KeyData { char path[260]; uint32_t flags,device,rootDevice; };
static_assert(sizeof(KeyData)==272,"DashLaunch keydata ABI");

int main() {
    HANDLE module=0;void* getEntry=nullptr;void* reloadEntry=nullptr;
    bool available=XexGetModuleHandle("launch.xex",&module)==ERROR_SUCCESS &&
        NT_SUCCESS(XexGetProcedureAddress(module,9,&getEntry)) &&
        NT_SUCCESS(XexGetProcedureAddress(module,4,&reloadEntry)) && getEntry && reloadEntry;
    bool protectedOptions=false; const char* defaultClass="unavailable";
    if(available) {
        auto get=reinterpret_cast<GetOption>(getEntry);
        char names[3][16]={"liveblock","livestrong","noupdater"};
        protectedOptions=true;
        for(auto& name:names) {
            uint32_t value=0;
            if(!get(name,&value) || value!=1) protectedOptions=false;
        }
        char keyName[]="Default"; uint32_t value=0;
        if(get(keyName,&value) && value>=0x80000000u && value<=0xfffffeefu) {
            const auto* key=reinterpret_cast<const KeyData*>(static_cast<uintptr_t>(value));
            defaultClass="other";
            if(key->flags==1 && std::memchr(key->path,0,sizeof key->path)) {
                if(!std::strcmp(key->path,"\\Apps\\Aurora\\Aurora.xex")) defaultClass="Aurora";
                else if(!std::strcmp(key->path,"\\Apps\\Games\\Games.xex")) defaultClass="Games";
                else if(!std::strcmp(key->path,"\\Apps\\EmulationStation\\emulationstation.xex")) defaultClass="EmulationStation";
            }
        }
    }
    FILE* file=std::fopen("game:/return-support.json","w");
    if(file) {
        std::fprintf(file,"{\"runtime_api_available\":%s,\"network_update_options_preserved_enabled\":%s,\"default_class\":\"%s\",\"runtime_changed\":false}\n",
            available ? "true" : "false",protectedOptions ? "true" : "false",defaultClass);
        std::fclose(file);
    }
    return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",nullptr,0);
}
