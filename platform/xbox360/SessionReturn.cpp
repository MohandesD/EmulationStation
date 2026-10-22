#include "SessionReturn.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <set>
#include <sstream>

namespace XboxReturn {
namespace {
std::string lower(std::string s) {
    for(auto& c:s) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
const Option* find(const Snapshot& s,const std::string& name) {
    for(const auto& o:s) if(lower(o.name)==lower(name)) return &o;
    return nullptr;
}
bool same(const Option& a,const Option& b) {
    return lower(a.name)==lower(b.name) && a.type==b.type && a.value==b.value &&
        lower(a.prefix)==lower(b.prefix) && lower(a.path)==lower(b.path) &&
        a.flags==b.flags && a.device==b.device && a.root==b.root;
}
bool protectedOptions(const Snapshot& s) {
    for(const char* name:{"liveblock","livestrong","noupdater"}) {
        const auto* o=find(s,name); if(!o || o->type!=0 || o->value!=1) return false;
    }
    return true;
}
bool makeActive(const Snapshot& original,const Option& target,Snapshot& active) {
    if(!valid(original) || !protectedOptions(original) || target.type!=7 || target.flags!=1 ||
       lower(target.prefix)!="hdd:" || target.path!="\\Apps\\EmulationStation\\emulationstation.xex") return false;
    active=original;
    for(const char* name:{"Default","BUT_Y"}) {
        bool found=false;
        for(auto& o:active) if(lower(o.name)==lower(name)) {
            if(o.type!=7) return false;
            const auto savedName=o.name; o=target; o.name=savedName; found=true;
        }
        if(!found) return false;
    }
    return valid(active);
}
bool pointsHome(const Snapshot& s,const Option& target) {
    for(const char* name:{"Default","BUT_Y"}) {
        const auto* o=find(s,name); if(!o) continue;
        Option expected=target; expected.name=o->name;
        if(same(*o,expected)) return true;
    }
    return false;
}
bool apply(Backend& b,const char* slot,const Snapshot& expected) {
    const auto text=ini(expected);
    if(text.empty() || !b.stage(slot,text)) return false;
    b.reload(slot);
    Snapshot observed;
    return b.read(observed) && equal(observed,expected);
}
}
bool valid(const Snapshot& s) {
    if(s.empty() || s.size()>256) return false;
    std::set<std::string> names;
    for(const auto& o:s) {
        if(o.name.empty() || o.name.size()>32 || o.name.find_first_not_of(
           "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=std::string::npos ||
           !names.insert(lower(o.name)).second || o.type>8) return false;
        if(o.type<6) {
            if(!o.path.empty() || !o.prefix.empty() || o.flags!=255 || o.device!=255 || o.root!=255 ||
               (o.type==0 && o.value>1) || (o.type>=1 && o.type<=3 && o.value>65535)) return false;
        } else {
            if(o.value!=0) return false;
            if(o.root==255) {
                if(!o.prefix.empty() || !o.path.empty() || o.flags!=255 || o.device!=255) return false;
            } else {
                if(o.root>=64 || (o.device>=64 && o.device!=255) || o.prefix.empty() ||
                   o.prefix.size()>30 || o.prefix.back()!=':' ||
                   o.prefix.substr(0,o.prefix.size()-1).find_first_not_of(
                     "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")!=std::string::npos ||
                   o.path.empty() || o.path[0]!='\\' || o.path.size()>=260 ||
                   o.path.find("..")!=std::string::npos || o.path.back()==' ' ||
                   o.path.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._- \\():$")!=std::string::npos) return false;
            }
        }
    }
    return true;
}
bool equal(const Snapshot& a,const Snapshot& b) {
    if(!valid(a) || !valid(b) || a.size()!=b.size()) return false;
    for(const auto& o:a) { const auto* p=find(b,o.name); if(!p || !same(o,*p)) return false; }
    return true;
}
std::string encode(const Snapshot& s) {
    if(!valid(s)) return {};
    std::ostringstream out; out<<"ES_SESSION_RETURN_1\n";
    for(const auto& o:s) out<<o.type<<' '<<o.name<<' '<<o.value<<' '<<o.flags<<' '<<o.device<<' '
        <<o.root<<' '<<std::quoted(o.prefix)<<' '<<std::quoted(o.path)<<'\n';
    return out.str();
}
bool decode(const std::string& text,Snapshot& s) {
    s.clear(); if(text.size()>131072) return false;
    std::istringstream in(text); std::string line;
    if(!std::getline(in,line) || line!="ES_SESSION_RETURN_1") return false;
    while(std::getline(in,line)) {
        Option o; std::istringstream row(line);
        if(!(row>>o.type>>o.name>>o.value>>o.flags>>o.device>>o.root>>std::quoted(o.prefix)>>std::quoted(o.path))) return false;
        row>>std::ws; if(!row.eof()) return false;
        s.push_back(o); if(s.size()>256) return false;
    }
    return valid(s);
}
std::string ini(const Snapshot& s) {
    if(!valid(s)) return {};
    // DashLaunch looks up keys independent of their section. Explicitly include
    // every runtime option: reload resets omitted values to defaults.
    std::ostringstream out; out<<"; Application-private temporary session; never install at a drive root.\n[Paths]\n";
    for(const auto& o:s) {
        out<<o.name<<" = ";
        if(o.type>=6) out<<o.prefix<<o.path;
        else if(o.type==0) out<<(o.value ? "true" : "false");
        else out<<"0x"<<std::hex<<o.value<<std::dec;
        out<<'\n';
    }
    return out.str();
}
bool arm(Backend& b,std::string& error) {
    error="COULD NOT PREPARE RETURN TO EMULATIONSTATION. THE GAME WAS NOT STARTED.";
    Snapshot before,active,original,observed; Option target;
    if(!b.read(before) || !valid(before) || !b.target(target)) return false;
    if(pointsHome(before,target)) {
        if(!b.loadOriginal(original) || !makeActive(original,target,active) || !equal(before,active)) return false;
        error.clear(); return true; // Re-entry after a game; keep the original restore point.
    }
    if(!makeActive(before,target,active) || !b.saveOriginal(before) ||
       !b.loadOriginal(original) || !equal(before,original) ||
       !b.stage("restore",ini(before)) || !b.stage("active",ini(active)) ||
       !b.read(observed) || !equal(before,observed)) return false;
    b.reload("active");
    if(b.read(observed) && equal(observed,active)) { error.clear(); return true; }
    // Reload has no success result. Verify every option, then restore on any mismatch.
    if(!apply(b,"restore",before)) error="SESSION RETURN COULD NOT BE RESTORED. STOP TESTING AND RESTART THE TEMPORARY SESSION.";
    return false;
}
bool restore(Backend& b,std::string& error) {
    error="COULD NOT RESTORE THE PREVIOUS DASHBOARD SETTINGS. STAY IN EMULATIONSTATION.";
    Snapshot current,original,active; Option target;
    if(!b.read(current) || !valid(current) || !b.target(target)) return false;
    if(!pointsHome(current,target)) { error.clear(); return true; } // A new session or no arm.
    if(!b.loadOriginal(original) || !makeActive(original,target,active) || !equal(current,active)) return false;
    if(!apply(b,"restore",original)) return false;
    error.clear(); return true;
}
}
