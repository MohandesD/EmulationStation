#include "SessionReturn.h"
#include <cassert>
#include <iostream>
#include <map>
using namespace XboxReturn;
static Option path(const char* name,const char* suffix) {
    Option o; o.name=name; o.type=7; o.prefix="Hdd:"; o.path=suffix;
    o.flags=1; o.device=3; o.root=3; return o;
}
static Snapshot initial() {
    Snapshot s;
    for(const char* name:{"liveblock","livestrong","noupdater"}) { Option o; o.name=name; o.value=1; s.push_back(o); }
    s.push_back(path("Default","\\Apps\\Aurora\\Aurora.xex"));
    Option y; y.name="BUT_Y"; y.type=7; s.push_back(y);
    auto plugin=path("plugin1","\\plugins\\debug.xex"); plugin.type=8; plugin.flags=0xFB; s.push_back(plugin);
    Option empty; empty.type=7; empty.name="BUT_A"; s.push_back(empty);
    Option timer; timer.type=5; timer.name="hddtimer"; timer.value=210; s.push_back(timer);
    return s;
}
struct Mock:Backend {
    Snapshot live=initial(),saved;
    std::map<std::string,Snapshot> stages;
    int reloads=0,saves=0;
    bool failSave=false,failStage=false,badActive=false,badRestore=false,changedWhileStaging=false;
    bool read(Snapshot& s) override {s=live;return true;}
    bool target(Option& o) override {o=path("Default","\\Apps\\EmulationStation\\emulationstation.xex");return true;}
    bool saveOriginal(const Snapshot& s) override {saves++;if(failSave)return false;saved=s;return true;}
    bool loadOriginal(Snapshot& s) override {s=saved;return !s.empty();}
    bool stage(const char* slot,const std::string& text) override {
        if(failStage)return false;
        // A deliberately independent INI reader models a runtime reload: numeric
        // and path values must survive serialization, not just pointer assignment.
        Snapshot s=saved;
        for(auto& o:s) {
            const auto prefix=o.name+" = "; const auto at=text.find(prefix);assert(at!=std::string::npos);
            const auto end=text.find('\n',at);const auto value=text.substr(at+prefix.size(),end-at-prefix.size());
            if(o.type>=6) {
                if(value.empty()) {o.prefix.clear();o.path.clear();o.flags=o.device=o.root=255;continue;}
                const auto colon=value.find(':');assert(colon!=std::string::npos);
                o.prefix=value.substr(0,colon+1);o.path=value.substr(colon+1);
                o.device=o.root=3;o.flags=o.type==7 ? 1 : 0xFE;
            } else if(o.type==0) o.value=value=="true" ? 1 : 0;
            else o.value=std::stoul(value,nullptr,0);
        }
        stages[slot]=s;
        if(changedWhileStaging)live[0].value=0;
        return true;
    }
    void reload(const char* slot) override {
        reloads++;live=stages.at(slot);
        if((std::string(slot)=="active" && badActive) || (std::string(slot)=="restore" && badRestore)) live[0].value=0;
    }
};
int main() {
    const auto before=initial();Snapshot decoded;std::string error;
    assert(valid(before));assert(decode(encode(before),decoded));assert(equal(before,decoded));
    assert(!decode(encode(before)+"truncated",decoded));
    auto bad=before;bad.push_back(before[0]);assert(!valid(bad));
    bad=before;bad[3].path="\\Apps\\bad\nDefault=evil";assert(!valid(bad));
    bad=before;bad[0].type=9;assert(!valid(bad));
    Mock m;assert(arm(m,error));assert(error.empty());assert(m.reloads==1);assert(!equal(m.live,before));
    assert(m.live[5].path==before[5].path);assert(m.live[7].value==210);
    assert(arm(m,error));assert(m.reloads==1 && m.saves==1); // next game does not overwrite restore point
    assert(restore(m,error));assert(equivalentAfterReload(before,m.live));assert(!equal(m.live,before));assert(m.reloads==2);
    assert(restore(m,error));assert(m.reloads==2); // new session / already restored
    Mock backup;backup.failSave=true;assert(!arm(backup,error));assert(backup.reloads==0);
    Mock disk;disk.failStage=true;assert(!arm(disk,error));assert(disk.reloads==0);
    Mock race;race.changedWhileStaging=true;assert(!arm(race,error));assert(race.reloads==0);
    Mock rollback;rollback.badActive=true;assert(!arm(rollback,error));assert(rollback.reloads==2);assert(equivalentAfterReload(before,rollback.live));
    Mock fatal;fatal.badActive=true;fatal.badRestore=true;assert(!arm(fatal,error));assert(error.find("STOP TESTING")!=std::string::npos);
    Mock absent;assert(arm(absent,error));absent.saved.clear();assert(!arm(absent,error));assert(!restore(absent,error));assert(absent.reloads==1);
    Mock altered;assert(arm(altered,error));altered.live.back().value=211;assert(!restore(altered,error));assert(altered.reloads==1);
    Mock restoreRace;assert(arm(restoreRace,error));restoreRace.changedWhileStaging=true;
    assert(!restore(restoreRace,error));assert(!error.empty());assert(restoreRace.reloads==1);
    assert(restoreRace.live[0].value==0); // preserve the external change, no reload
    Mock unsafe;unsafe.live[0].value=0;assert(!arm(unsafe,error));assert(unsafe.reloads==0);
    Mock stale;stale.saved=before;stale.saved.back().value=9;assert(arm(stale,error));assert(stale.saved.back().value==210);
    auto pluginStatus=before;pluginStatus[5].flags=0xFE;
    assert(equivalentAfterReload(before,pluginStatus));assert(!equivalentAfterReload(pluginStatus,before));
    pluginStatus[5].flags=255;assert(!equivalentAfterReload(before,pluginStatus));
    pluginStatus=before;pluginStatus[3].flags=0xFE;assert(!equivalentAfterReload(before,pluginStatus));
    pluginStatus=before;pluginStatus[5].flags=0xFE;pluginStatus[5].path="\\changed.xex";assert(!equivalentAfterReload(before,pluginStatus));
    std::cout<<"Session snapshot, re-entry, preservation, rejection and rollback checks passed\n";
}
