// Clean-package model/context/theme validation, with inert text files, no XEX execution.
#include "SystemData.h"
#include "Settings.h"
#include "ThemeData.h"
#include "AudioManager.h"
#include "Log.h"
#include "resources/ResourceManager.h"
#include "LaunchCommand.h"
#include "SalviaLaunch.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs=std::filesystem;
std::string getHomePath(){return "game:";}
AudioManager::~AudioManager(){assert(false && "No audio runtime in package model test");}
GuiComponent::~GuiComponent(){assert(false && "No UI runtime in package model test");}
// Only resource existence is stubbed; the production theme parser is linked.
ResourceManager::ResourceManager(){}
std::shared_ptr<ResourceManager> ResourceManager::sInstance;
std::shared_ptr<ResourceManager>& ResourceManager::getInstance(){
    if(!sInstance)sInstance=std::shared_ptr<ResourceManager>(new ResourceManager());
    return sInstance;
}
bool ResourceManager::fileExists(const std::string& path)const{
    return path==":/opensans_hebrew_condensed_regular.ttf" || fs::is_regular_file(path);
}
static void write(const fs::path& p,const std::string& text){
    fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f<<text;assert(f);
}
static std::string read(const fs::path& p){
    std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char**argv){
    assert(argc==4);
    const auto package=fs::absolute(argv[1]),salvia=fs::absolute(argv[2]),fixture=fs::absolute(argv[3]);
    assert(!fs::exists(fixture));fs::create_directories(fixture);fs::current_path(fixture);
    fs::copy(package/"Apps/EmulationStation","game:",fs::copy_options::recursive);
    fs::create_directories("hdd:/Emulators/Salvia");
    fs::copy(salvia/"config","hdd:/Emulators/Salvia/config",fs::copy_options::recursive);
    fs::copy_file(salvia/"salvia.cfg","hdd:/Emulators/Salvia/salvia.cfg");
    Log::open();auto* settings=Settings::getInstance();
    assert(!settings->getBool("XboxLaunchEnabled") && !settings->getBool("XboxSessionReturn"));
    assert(settings->getString("XboxTestRom").empty() && settings->getInt("XboxDiagnosticFrames")==0);
    assert(!settings->getBool("XboxDiagnosticBrowse") && !settings->getBool("XboxDiagnosticHandoff"));
    assert(settings->getString("ThemeSet")=="clean");
    assert(SystemData::loadConfig());assert(SystemData::sSystemVector.empty());
    pugi::xml_document doc;assert(doc.load_file("game:/.emulationstation/es_systems.cfg"));
    unsigned expectedSystems=0,expectedGames=0,missingMenuRejected=0;
    for(auto sys:doc.child("systemList").children("system")){
        ++expectedSystems;
        ThemeData theme;theme.loadFile("game:/.emulationstation/themes/clean/"+
            std::string(sys.child("theme").text().get())+"/theme.xml");
        assert(theme.getElement("basic","gamelist","textlist"));
        assert(theme.getElement("system","help","helpsystem"));
        auto check=[&](pugi::xml_node route){
            std::istringstream exts(route.child("extension").text().get());std::string extension;exts>>extension;
            std::string rom=std::string(route.child("path").text().get())+"/Package test fixture"+extension;
            write(rom,"INERT CONFIGURATION TEST TEXT. NOT A ROM. NEVER EXECUTE.\n");
            std::string command=route.child("command").text().get();auto pos=command.find("%ROM%");assert(pos!=std::string::npos);
            command.replace(pos,5,"\""+rom+"\"");XboxLaunchCommand parsed;std::string error;
            assert(parseXboxLaunchCommand(command,parsed,error));
            assert(!isXboxLaunchPermitted(parsed,false,""));
            const auto core=salvia/fs::path(parsed.executable).filename();assert(fs::is_regular_file(core));
            if(!fs::exists(parsed.executable))fs::copy_file(core,parsed.executable);
            assert(validateXboxLaunchFiles(parsed,error));
            if(!fs::exists("hdd:/Emulators/Salvia/menu.tmp")){
                assert(!validateSalviaLaunch(parsed,error));++missingMenuRejected;
            }
            // An inert test-only menu structure, never shipped or sent to a console.
            write("hdd:/Emulators/Salvia/menu.tmp",std::string(8496,'\0'));
            assert(validateSalviaLaunch(parsed,error));
            assert(prepareSalviaLaunch(parsed,error));assert(validateSalviaLaunch(parsed,error,true));
            ++expectedGames;
        };
        if(auto routes=sys.child("xboxRoutes")){for(auto route:routes.children("route"))check(route);}
        else check(sys);
    }
    assert(missingMenuRejected==1);
    assert(SystemData::loadConfig());assert(SystemData::sSystemVector.size()==expectedSystems);
    unsigned games=0;
    for(auto* sys:SystemData::sSystemVector){games+=sys->getGameCount();
        assert(sys->getTheme()->getElement("basic","gamelist","textlist"));}
    assert(games==expectedGames);
    assert(read("hdd:/Emulators/Salvia/salvia.cfg")==read(salvia/"salvia.cfg"));
    SystemData::deleteSystems();Log::close();
    std::cout<<"{\"sections\":"<<expectedSystems<<",\"routes\":"<<expectedGames
             <<",\"empty_library_confirmed\":true,\"missing_menu_rejected\":true,\"production_theme_parser\":true,\"console_contacted\":false}\n";
}
