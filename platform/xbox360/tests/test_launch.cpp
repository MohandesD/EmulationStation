#include "LaunchCommand.h"
#include <cassert>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>

int main() {
    XboxLaunchCommand result; std::string error;
    assert(parseXboxLaunchCommand("xex \"Hdd1:\\Emulators\\Salvia\\default.xex\" \"hdd:/ROMs/nes/Game's name.nes\"",result,error));
    assert(result.executable=="hdd:/Emulators/Salvia/default.xex");
    assert(result.rom=="Hdd:\\ROMs\\nes\\Game's name.nes");
    assert(!isXboxLaunchPermitted(result,false,""));
    assert(isXboxLaunchPermitted(result,true,""));
    assert(isXboxLaunchPermitted(result,false,"hdd:/ROMs/nes/Game's name.nes"));
    assert(!isXboxLaunchPermitted(result,false,"hdd:/ROMs/nes/another.nes"));
    assert(!isXboxLaunchPermitted(result,false,"hdd:/ROMs/nes/../nes/Game's name.nes"));
    assert(parseXboxLaunchCommand("xex usb:/Apps/Test.XEX Usb0:/ROMs/test.zip",result,error));
    assert(result.rom=="Usb0:\\ROMs\\test.zip");
    assert(parseXboxLaunchCommand("  xex   game:/app.xex  ",result,error));
    assert(result.rom.empty());
    assert(!isXboxLaunchPermitted(result,false,"hdd:/ROMs/test.nes"));
    const char* invalid[]={"", "xex", "shell hdd:/a.xex", "xex hdd:/a.elf", "xex hdd:/a.xex extra extra",
        "xex \"hdd:/a.xex", "xex hdd:/a.xex \"\"", "xex hdd:/a.xex game:/a.nes", "xex hdd:/a.xex /a.nes",
        "xex \"hdd:/a.xex\"tail", "xex hdd:/../a.xex", "xex hdd:/./a.xex", "xex hdd://a.xex",
        "xex unknown:/a.xex", "xex hdd:/a.xex\n", "xex hdd:/a.xex hdd:/r*ms/a.nes", "xex hdd:/a.xex hdd:/a:/b.nes",
        "xex hdd:/a.xex dvd:/test.nes"};
    for(const char* text:invalid) { assert(!parseXboxLaunchCommand(text,result,error)); assert(!error.empty()); assert(result.executable.empty()); }
    assert(!parseXboxLaunchCommand("xex hdd:/"+std::string(39,'a')+".xex",result,error));
    assert(parseXboxLaunchCommand("xex hdd:/"+std::string(38,'a')+".xex",result,error));

    // Exercise the actual filesystem preflight with missing, invalid, empty,
    // directory and available file fixtures. These are not real game/XEX files.
    namespace fs=std::filesystem;
    const auto before=fs::current_path();
    std::string pattern=(fs::temp_directory_path()/"es-launch-test-XXXXXX").string();
    char* temporary=mkdtemp(&pattern[0]); assert(temporary);
    const auto directory=fs::absolute(temporary);fs::current_path(directory);
    fs::create_directories("hdd:/Apps");fs::create_directories("hdd:/Games");
    assert(parseXboxLaunchCommand("xex hdd:/Apps/test.xex hdd:/Games/test.nes",result,error));
    assert(!validateXboxLaunchFiles(result,error));
    { std::ofstream file(result.executable);file<<std::string(24,'x'); }
    assert(!validateXboxLaunchFiles(result,error));
    { std::ofstream file(result.executable);file<<"XEX2"<<std::string(20,'\0'); }
    assert(!validateXboxLaunchFiles(result,error));
    fs::create_directory("hdd:/Games/test.nes");
    assert(!validateXboxLaunchFiles(result,error));
    fs::remove("hdd:/Games/test.nes");
    { std::ofstream file("hdd:/Games/test.nes"); }
    assert(!validateXboxLaunchFiles(result,error));
    { std::ofstream file("hdd:/Games/test.nes");file<<"test fixture"; }
    assert(validateXboxLaunchFiles(result,error) && error.empty());
    assert(parseXboxLaunchCommand("xex hdd:/Apps/test.xex",result,error));
    assert(validateXboxLaunchFiles(result,error));
    fs::current_path(before);fs::remove_all(directory);
}
