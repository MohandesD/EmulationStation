#include "XboxRoutes.h"
#include "LaunchCommand.h"
#include <algorithm>
#include <cctype>
#include <set>

namespace {
std::string lower(std::string value) {
    for(auto& c:value) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
bool normalize(const std::string& input,std::string& result) {
    if(input.empty() || input.size()>900 || input.find_first_of("\r\n\t\"<>|?*")!=std::string::npos) return false;
    for(unsigned char c:input) if(c<32 || c==127) return false;
    std::string value=input;
    std::replace(value.begin(),value.end(),'\\','/');
    const auto colon=value.find(':');
    if(colon==std::string::npos || colon+1>=value.size() || value[colon+1]!='/') return false;
    std::string drive=lower(value.substr(0,colon));
    if(drive=="hdd1") drive="hdd";
    if(drive=="usb0") drive="usb";
    if(drive!="hdd" && drive!="usb" && drive!="usb1" && drive!="usb2") return false;
    std::string tail=value.substr(colon+2);
    if(tail.empty() || tail.find(':')!=std::string::npos) return false;
    if(tail.back()=='/') tail.pop_back();
    if(tail.empty()) return false;
    for(size_t start=0;start<tail.size();) {
        size_t end=tail.find('/',start);if(end==std::string::npos) end=tail.size();
        const auto part=tail.substr(start,end-start);
        if(part.empty() || part=="." || part==".." || part.size()>42 || part.back()==' ' || part.back()=='.') return false;
        start=end+1;
    }
    if(tail.back()=='/') return false;
    result=drive+":/"+tail;
    return true;
}
bool beneath(const std::string& child,const std::string& parent) {
    return child.size()>parent.size() && child.compare(0,parent.size(),parent)==0 && child[parent.size()]=='/';
}
bool extension(const std::string& value) {
    if(value.size()<2 || value.size()>17 || value.front()!='.') return false;
    return std::all_of(value.begin()+1,value.end(),[](unsigned char c){return std::isalnum(c)!=0;});
}
}

std::string xboxRoutePathKey(const std::string& path) {
    std::string value;return normalize(path,value) ? lower(value) : std::string();
}
bool validateXboxRoutes(const std::string& root,std::vector<XboxRoute>& routes,std::string& error) {
    error="INVALID XBOX GAME SOURCE CONFIGURATION.";
    const auto rootKey=xboxRoutePathKey(root);
    if(rootKey.empty() || routes.empty() || routes.size()>256) return false;
    auto validated=routes;
    std::vector<std::string> roots;
    for(auto& route:validated) {
        std::string path;
        if(!normalize(route.path,path)) return false;
        const auto key=lower(path);
        if(!beneath(key,rootKey)) return false;
        for(const auto& previous:roots)
            if(key==previous || beneath(key,previous) || beneath(previous,key)) return false;
        roots.push_back(key);route.path=path;
        if(route.extensions.empty()) return false;
        std::set<std::string> extensions;
        for(const auto& ext:route.extensions) {
            if(!extension(ext)) return false;
            extensions.insert(lower(ext));
        }
        route.extensions.assign(extensions.begin(),extensions.end());
        const auto placeholder=route.command.find("%ROM%");
        if(placeholder==std::string::npos || route.command.find("%ROM%",placeholder+5)!=std::string::npos) return false;
        auto sample=route.command;
        const auto fixture=path+"/es-route-check"+route.extensions.front();
        sample.replace(placeholder,5,"\""+fixture+"\"");
        if(sample.find('%')!=std::string::npos) return false;
        XboxLaunchCommand parsed;std::string parseError;
        if(!parseXboxLaunchCommand(sample,parsed,parseError) || xboxRoutePathKey(parsed.rom)!=xboxRoutePathKey(fixture)) return false;
    }
    routes=std::move(validated);error.clear();return true;
}
const XboxRoute* resolveXboxRoute(const std::vector<XboxRoute>& routes,const std::string& path) {
    const auto key=xboxRoutePathKey(path);
    if(key.empty()) return nullptr;
    const auto slash=key.find_last_of('/'),dot=key.find_last_of('.');
    if(dot==std::string::npos || dot<=slash || dot+1==key.size()) return nullptr;
    const auto ext=key.substr(dot);
    const XboxRoute* selected=nullptr;
    for(const auto& route:routes) {
        const auto source=xboxRoutePathKey(route.path);
        if(source.empty() || !beneath(key,source)) continue;
        if(std::find(route.extensions.begin(),route.extensions.end(),ext)==route.extensions.end()) continue;
        if(selected) return nullptr; // Defensive rejection even if unvalidated routes are supplied.
        selected=&route;
    }
    return selected;
}
bool resolveXboxRouteMetadataPath(const std::string& root,const std::string& entry,std::string& path) {
    path.clear();std::string common;
    if(!normalize(root,common)) return false;
    std::string value=entry;
    std::replace(value.begin(),value.end(),'\\','/');
    if(value.compare(0,2,"./")==0) value=common+"/"+value.substr(2);
    std::string resolved;
    if(!normalize(value,resolved) || !beneath(lower(resolved),lower(common))) return false;
    path=resolved;return true;
}
std::string makeXboxRouteRelativePath(const std::string& root,const std::string& path) {
    std::string common,value;
    if(!normalize(root,common) || !normalize(path,value) || !beneath(lower(value),lower(common))) return {};
    return "./"+value.substr(common.size()+1);
}
