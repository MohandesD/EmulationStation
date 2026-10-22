#include "FrontendState.h"
#include "XboxRoutes.h"
#include "SystemData.h"
#include "views/ViewController.h"
#include "pugixml/pugixml.hpp"
#include "platform.h"
#include "Log.h"
#include <cstdio>

static std::string statePath() { return getHomePath()+"/.emulationstation/es_selection.cfg"; }
bool rememberXboxSelection(FileData* launching) {
    auto controller=ViewController::get();
    const auto& state=controller->getState();
    if(state.viewing!=ViewController::SYSTEM_SELECT && state.viewing!=ViewController::GAME_LIST) return false;
    SystemData* system=launching ? launching->getSystem() : state.getSystem();
    if(!system) return false;
    const bool gameList=launching || state.viewing==ViewController::GAME_LIST;
    FileData* selected=launching;
    if(!selected && gameList) selected=controller->getGameListView(system)->getCursor();
    pugi::xml_document document;
    auto root=document.append_child("selection");
    root.append_attribute("version")=1;
    root.append_attribute("system")=system->getName().c_str();
    root.append_attribute("view")=gameList ? "games" : "systems";
    if(selected) root.append_attribute("path")=selected->getPath().generic_string().c_str();
    const std::string path=statePath(),temporary=path+".tmp";
    if(!document.save_file(temporary.c_str()) || std::rename(temporary.c_str(),path.c_str())!=0) {
        LOG(LogWarning)<<"Could not save frontend selection"; return false;
    }
    return true;
}
bool restoreXboxSelection() {
    FILE* file=std::fopen(statePath().c_str(),"rb");
    if(!file) return false;
    char data[8193]; const size_t bytes=std::fread(data,1,sizeof data,file);
    const bool readable=!std::ferror(file); std::fclose(file);
    if(!readable || bytes>8192) return false;
    pugi::xml_document document;
    if(!document.load_buffer(data,bytes)) return false;
    auto root=document.child("selection");
    if(root.attribute("version").as_int()!=1) return false;
    const std::string name=root.attribute("system").as_string();
    const std::string path=root.attribute("path").as_string();
    const std::string pathKey=xboxRoutePathKey(path);
    auto controller=ViewController::get();
    for(auto system:SystemData::sSystemVector) {
        if(system->getName()!=name) continue;
        if(std::string(root.attribute("view").as_string())=="games") {
            for(auto game:system->getRootFolder()->getFilesRecursive(GAME|FOLDER)) {
                const auto current=game->getPath().generic_string();
                if(current!=path && (pathKey.empty() || xboxRoutePathKey(current)!=pathKey)) continue;
                controller->getGameListView(system)->setCursor(game);
                controller->goToGameList(system);
                LOG(LogInfo)<<"Restored game-list selection";
                return true;
            }
        }
        controller->goToSystemView(system);
        LOG(LogInfo)<<"Restored system selection";
        return true;
    }
    return false;
}
