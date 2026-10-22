#include "SalviaLaunch.h"
#include <xsdk/xam.h>
#include <cstdio>

// Does not launch a core or read a ROM. The text fixture is just launch data
// syntax for the shared menu-state helper; only the final Aurora handoff runs.
int main() {
    XboxLaunchCommand cmd; std::string error;
    const bool parsed=parseXboxLaunchCommand("salvia nes hdd:/Emulators/Salvia/Nestopia.xex hdd:/Apps/ESProbe/handoff-fixture.txt",cmd,error);
    const bool valid=parsed && validateSalviaLaunch(cmd,error);
    const bool prepared=valid && prepareSalviaLaunch(cmd,error);
    const bool ready=prepared && validateSalviaLaunch(cmd,error,true);
    FILE* file=std::fopen("game:/salvia-context-result.txt","wb");
    if(file) {
        std::fprintf(file,"parsed=%d\nvalid=%d\nprepared=%d\nready=%d\nno_core_or_game_launch=1\n",parsed,valid,prepared,ready);
        std::fclose(file);
    }
    return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",nullptr,0);
}
