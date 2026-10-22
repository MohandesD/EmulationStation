// A diagnostic receiver, never an emulator: verifies the launch ABI and game:
// directory after the real frontend has shut down, then returns to Aurora.
#include <xsdk/xam.h>
#include <cstdio>
#include <cstring>

static bool fixtureMatches(const char* path) {
    FILE* file=std::fopen(path,"rb");
    if(!file) return false;
    char content[64]{};
    size_t bytes=std::fread(content,1,sizeof content,file);
    bool ok=!std::ferror(file); std::fclose(file);
    const char expected[]="EmulationStation handoff test\n";
    return ok && bytes==sizeof(expected)-1 && !std::memcmp(content,expected,bytes);
}
int main() {
    const char expected[]="Hdd:\\Apps\\ESProbe\\handoff-fixture.txt";
    char launchData[256]{};
    size_t bytes=xsdk_launch_data(launchData,sizeof launchData);
    bool exact=bytes==sizeof expected && !std::memcmp(launchData,expected,sizeof expected);
    bool relative=fixtureMatches("game:/handoff-fixture.txt");
    bool absolute=fixtureMatches("hdd:/Apps/ESProbe/handoff-fixture.txt");
    FILE* report=std::fopen("hdd:/Apps/ESProbe/handoff-result.json","w");
    if(report) {
        std::fprintf(report,"{\"launch_data_exact\":%s,\"game_mount_correct\":%s,\"absolute_file_read\":%s,\"rom_launched\":false}\n",
            exact ? "true" : "false", relative ? "true" : "false", absolute ? "true" : "false");
        std::fclose(report);
    }
    return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",nullptr,0);
}
