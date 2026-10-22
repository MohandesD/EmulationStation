#include "LaunchCommand.h"
#include <cassert>
#include <string>

int main() {
    XboxLaunchCommand result; std::string error;
    assert(parseXboxLaunchCommand("xex \"Hdd1:\\Emulators\\Salvia\\default.xex\" \"hdd:/ROMs/nes/Game's name.nes\"",result,error));
    assert(result.executable=="hdd:/Emulators/Salvia/default.xex");
    assert(result.rom=="Hdd:\\ROMs\\nes\\Game's name.nes");
    assert(parseXboxLaunchCommand("xex usb:/Apps/Test.XEX Usb0:/ROMs/test.zip",result,error));
    assert(result.rom=="Usb0:\\ROMs\\test.zip");
    assert(parseXboxLaunchCommand("  xex   game:/app.xex  ",result,error));
    assert(result.rom.empty());
    const char* invalid[]={"", "xex", "shell hdd:/a.xex", "xex hdd:/a.elf", "xex hdd:/a.xex extra extra",
        "xex \"hdd:/a.xex", "xex hdd:/a.xex \"\"", "xex hdd:/a.xex game:/a.nes", "xex hdd:/a.xex /a.nes",
        "xex \"hdd:/a.xex\"tail", "xex hdd:/../a.xex", "xex hdd:/./a.xex", "xex hdd://a.xex",
        "xex unknown:/a.xex", "xex hdd:/a.xex\n", "xex hdd:/a.xex hdd:/r*ms/a.nes", "xex hdd:/a.xex hdd:/a:/b.nes"};
    for(const char* text:invalid) { assert(!parseXboxLaunchCommand(text,result,error)); assert(!error.empty()); assert(result.executable.empty()); }
    assert(!parseXboxLaunchCommand("xex hdd:/"+std::string(39,'a')+".xex",result,error));
    assert(parseXboxLaunchCommand("xex hdd:/"+std::string(38,'a')+".xex",result,error));
}
