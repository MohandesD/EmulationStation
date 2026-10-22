#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace XboxReturn {
struct Option {
    std::string name, prefix, path;
    uint32_t type=0, value=0, flags=255, device=255, root=255;
};
using Snapshot=std::vector<Option>;
// All writes/reloads are confined to application-private session directories.
// The implementation never writes an automatically discovered boot launch.ini.
struct Backend {
    virtual ~Backend()=default;
    virtual bool read(Snapshot&)=0;
    virtual bool target(Option&)=0;
    virtual bool saveOriginal(const Snapshot&)=0;
    virtual bool loadOriginal(Snapshot&)=0;
    virtual bool stage(const char* slot,const std::string& ini)=0;
    virtual void reload(const char* slot)=0;
};
bool valid(const Snapshot&);
bool equal(const Snapshot&,const Snapshot&);
// Runtime reload reclassifies an already-loaded plugin (0xFB) as found (0xFE).
// Configuration/path/device equality is still required; other flag changes fail.
bool equivalentAfterReload(const Snapshot& expected,const Snapshot& observed);
std::string encode(const Snapshot&);
bool decode(const std::string&,Snapshot&);
std::string ini(const Snapshot&);
bool arm(Backend&,std::string& error);
bool restore(Backend&,std::string& error);
}

// Resolve the already-loaded runtime. Does not change options or files.
std::unique_ptr<XboxReturn::Backend> openXboxReturnRuntime();

// Disabled unless explicitly enabled for a later hardware test.
bool prepareXboxSessionReturn(std::string& error);
bool restoreXboxSessionReturn(std::string& error);
