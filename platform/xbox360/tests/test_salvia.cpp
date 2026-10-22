#include "SalviaLaunch.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iterator>

static void put(const std::string& p,const std::string& bytes) { std::ofstream f(p,std::ios::binary); f<<bytes; assert(f.good()); }
static std::string get(const std::string& p) { std::ifstream f(p,std::ios::binary); assert(f.good()); return {std::istreambuf_iterator<char>(f),{}}; }
int main() {
    namespace fs=std::filesystem;
    const auto before=fs::current_path();
    std::string pattern=(fs::temp_directory_path()/"es-salvia-test-XXXXXX").string();
    char* temporary=mkdtemp(&pattern[0]); assert(temporary);
    const auto directory=fs::absolute(temporary); fs::current_path(directory);
    const std::string root="hdd:/Salvia/"; fs::create_directories(root+"config");
    const std::string main="emulators=nes snes fbn\n";
    const std::string nes="directory=\nexecutable=Nestopia.xex;FCEU.xex\nno_uncompress=no\n";
    put(root+"salvia.cfg",main); put(root+"config/nes.cfg",nes);
    // Synthetic Xbox ABI state: Arcade selected, with stale navigation/filter data.
    // Layout/background should survive; no ROM or private capture is embedded.
    std::string menu(8496,'\0'); menu[3]=2; menu[7]=7; menu[15]=9;
    menu[23]=1; menu[24]=1; menu[31]=5; menu[40]=1; menu[4397]='x';
    put(root+"menu.tmp",menu);
    XboxLaunchCommand cmd; std::string error;
    assert(parseXboxLaunchCommand("salvia nes hdd:/Salvia/Nestopia.xex hdd:/ROMs/nes/test.nes",cmd,error));
    assert(cmd.salviaSystem=="nes");
    assert(!isXboxLaunchPermitted(cmd,false,""));
    assert(isXboxLaunchPermitted(cmd,false,"hdd:/ROMs/nes/test.nes"));
    assert(validateSalviaLaunch(cmd,error));
    assert(get(root+"menu.tmp")==menu && !fs::exists(root+"menu.es-backup"));
    assert(!validateSalviaLaunch(cmd,error,true));
    assert(prepareSalviaLaunch(cmd,error));
    assert(validateSalviaLaunch(cmd,error,true));
    assert(get(root+"menu.es-backup")==menu);
    auto selected=get(root+"menu.tmp");
    assert(selected.size()==8496 && selected[3]==0 && selected[23]==1 && selected[24]==1);
    assert(selected[7]==0 && selected[15]==0 && selected[31]==0 && selected[40]==0 && selected[4397]==0);
    assert(prepareSalviaLaunch(cmd,error));
    assert(get(root+"menu.es-backup")==menu); // Idempotent, don't erase rollback.
    // Upstream skips configurations that cannot open. With only nes.cfg
    // present, "missing nes" places NES at Salvia index 0, not token index 1.
    // Reject without rewriting either saved menu or its verified backup.
    const auto savedBackup=get(root+"menu.es-backup");
    auto rejectList=[&](const std::string& list) {
        put(root+"salvia.cfg","emulators="+list+"\n");
        assert(!validateSalviaLaunch(cmd,error));
        assert(!prepareSalviaLaunch(cmd,error));
        assert(get(root+"menu.tmp")==selected);
        assert(get(root+"menu.es-backup")==savedBackup);
    };
    rejectList("missing nes");
    fs::create_directory(root+"config/unreadable.cfg");
    rejectList("unreadable nes"); // A directory cannot be opened as a config.
    // Literal-space splitting preserves empty tokens and tabs within names.
    // Neither may be collapsed to a plausible but different positional list.
    put(root+"config/first.cfg","");
    for(const char* list:{"first  nes", "first\tnes", "../first nes", "first/child nes", "first:first nes", "first nes invalid!"})
        rejectList(list);
    // An empty readable predecessor still occupies one upstream menu index.
    // Missing entries after the target do not change the target's position.
    put(root+"salvia.cfg","emulators=first nes missing\n");
    assert(validateSalviaLaunch(cmd,error));
    assert(prepareSalviaLaunch(cmd,error));
    assert(get(root+"menu.tmp")[3]==1);
    assert(validateSalviaLaunch(cmd,error,true));
    put(root+"salvia.cfg",main);
    put(root+"menu.tmp",selected);
    put(root+"menu.es-backup",savedBackup);
    // Wrong/unknown core, malformed config, foreign/truncated ABI: no mutation.
    for(const auto& bad:{std::string("executable=Other.xex\nno_uncompress=no\n"),nes+"executable=Other.xex\n"}) {
        put(root+"config/nes.cfg",bad);
        assert(!prepareSalviaLaunch(cmd,error)); assert(get(root+"menu.tmp")==selected);
    }
    put(root+"config/nes.cfg",nes);
    put(root+"salvia.cfg","emulators=snes fbn\n"); assert(!prepareSalviaLaunch(cmd,error));
    put(root+"salvia.cfg",main);
    for(const auto& bad:{std::string(8495,'\0'),std::string(8497,'\0'),std::string(8496,'x')}) {
        put(root+"menu.tmp",bad); assert(!prepareSalviaLaunch(cmd,error)); assert(get(root+"menu.tmp")==bad);
    }
    put(root+"menu.tmp",menu);
    // A failed backup must not alter the original or permit the launch.
    fs::remove(root+"menu.es-backup"); fs::create_directory(root+"menu.es-backup");
    assert(!prepareSalviaLaunch(cmd,error)); assert(get(root+"menu.tmp")==menu);
    assert(parseXboxLaunchCommand("xex hdd:/Salvia/Nestopia.xex hdd:/ROMs/nes/test.nes",cmd,error));
    assert(!prepareSalviaLaunch(cmd,error)); assert(get(root+"menu.tmp")==menu);
    for(const char* invalid:{"salvia", "salvia ../nes hdd:/s.xex hdd:/r.nes", "salvia nes hdd:/s.xex", "salvia nes hdd:/s.xex hdd:/r.nes extra"})
        assert(!parseXboxLaunchCommand(invalid,cmd,error));
    assert(parseXboxLaunchCommand("xex hdd:/Apps/receiver.xex hdd:/fixture.txt",cmd,error));
    assert(validateSalviaLaunch(cmd,error,true) && prepareSalviaLaunch(cmd,error));
    assert(get(root+"menu.tmp")==menu); // Non-Salvia diagnostic remains independent.
    fs::current_path(before); fs::remove_all(directory);
}
