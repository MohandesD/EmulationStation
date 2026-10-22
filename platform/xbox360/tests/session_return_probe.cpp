// No games. Default mode only reads the runtime. An explicit app-local mode
// file containing "verify" exercises arm + immediate verified restoration.
#include "SessionReturn.h"
#include <xsdk/xam.h>
#include <cstdio>
#include <cstring>

static bool save(const char* name,const std::string& data) {
    const auto path=std::string("game:/")+name;
    FILE* f=std::fopen(path.c_str(),"wb"); if(!f) return false;
    const bool ok=std::fwrite(data.data(),1,data.size(),f)==data.size();
    return std::fclose(f)==0 && ok;
}
int main() {
    char mode[16]{};FILE* f=std::fopen("game:/mode.txt","rb");
    if(f) { std::fread(mode,1,sizeof(mode)-1,f);std::fclose(f); }
    const bool verify=std::strcmp(mode,"verify\n")==0;
    save("phase.txt","reading\n");
    auto runtime=openXboxReturnRuntime();
    XboxReturn::Snapshot before,after; XboxReturn::Option target;std::string error;
    const bool readable=runtime && runtime->read(before);
    const bool targetValid=readable && runtime->target(target);
    const bool saved=readable && save("before.state",XboxReturn::encode(before));
    bool armed=false,restored=false,unchanged=false;
    if(verify && targetValid && saved) {
        save("phase.txt","arming\n");
        armed=XboxReturn::arm(*runtime,error);
        save("phase.txt","restoring\n");
        // arm performs its own rollback on failure; still inspect the result.
        restored=armed ? XboxReturn::restore(*runtime,error) : false;
        if(runtime->read(after)) {
            save("after.state",XboxReturn::encode(after));
            unchanged=XboxReturn::equal(before,after);
        }
    }
    char result[384];
    std::snprintf(result,sizeof result,"{\"mode\":\"%s\",\"runtime_readable\":%s,\"options\":%u,\"target_valid\":%s,\"snapshot_saved\":%s,\"armed\":%s,\"restored\":%s,\"all_options_unchanged\":%s,\"rom_launched\":false}\n",
        verify?"verify":"inspect",readable?"true":"false",static_cast<unsigned>(before.size()),
        targetValid?"true":"false",saved?"true":"false",armed?"true":"false",
        restored?"true":"false",unchanged?"true":"false");
    save("result.json",result);save("error.txt",error);save("phase.txt","complete\n");
    return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",nullptr,0);
}
