#include "XboxRoutes.h"
#include <cassert>
#include <iostream>

static std::vector<XboxRoute> sources() {
    return {
        {"hdd:/ROMs/fbneo",{".zip",".ZIP"},"salvia fbn \"hdd:/Emulators/Salvia/fbn.xex\" %ROM%"},
        {"hdd:/ROMs/mame2003",{".zip"},"salvia mame \"hdd:/Emulators/Salvia/mame.xex\" %ROM%"}
    };
}
int main() {
    std::string error;
    auto routes=sources();
    assert(validateXboxRoutes("hdd:/ROMs",routes,error) && error.empty());
    assert(routes[0].extensions.size()==1 && routes[0].extensions[0]==".zip");
    assert(resolveXboxRoute(routes,"hdd:/ROMs/fbneo/xmen.zip")==&routes[0]);
    assert(resolveXboxRoute(routes,"HDD1:\\ROMS\\FBNEO\\XMEN.ZiP")==&routes[0]);
    assert(resolveXboxRoute(routes,"hdd:/ROMs/mame2003/xmen.zip")==&routes[1]);
    assert(resolveXboxRoute(routes,"hdd:/ROMs/mame2003/subfolder/xmen.zip")==&routes[1]);
    assert(routes[0].command!=routes[1].command);
    assert(xboxRoutePathKey("hdd:/ROMs/fbneo/xmen.zip")!=xboxRoutePathKey("hdd:/ROMs/mame2003/xmen.zip"));
    assert(xboxRoutePathKey("HDD1:\\ROMS\\FBNEO\\XMEN.ZIP")==xboxRoutePathKey("hdd:/ROMs/fbneo/xmen.zip"));
    const char* rejected[]={
        "hdd:/ROMs/fbneoplus/xmen.zip","hdd:/ROMs/fbneo.zip","hdd:/ROMs/other/xmen.zip",
        "hdd:/ROMs/fbneo/xmen.nes","hdd:/ROMs/mame2003/xmen.bin","usb:/ROMs/fbneo/xmen.zip",
        "hdd:/ROMs/fbneo/../mame2003/xmen.zip","hdd:/ROMs/fbneo/./xmen.zip",
        "hdd:/ROMs//fbneo/xmen.zip","./fbneo/xmen.zip","hdd:/ROMs/fbneo/xmen.zip\n",
        "hdd:/ROMs/fbneo/xmen.zip ","hdd:/ROMs/fbneo/xmen.zip.","hdd:/ROMs/fbneo/xmen.zip:other"
    };
    for(const auto* path:rejected) assert(!resolveXboxRoute(routes,path));

    auto bad=sources();bad[1].path="hdd:/ROMs/fbneo/other";
    assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    bad=sources();bad[0].path="hdd:/ROMs/mame2003/sub";
    assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    bad=sources();bad[1].path="HDD1:\\ROMS\\FBNEO\\";
    assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    assert(bad[1].path=="HDD1:\\ROMS\\FBNEO\\"); // Failed validation is not a partial edit.
    for(const auto* path:{"hdd:/ROMs","hdd:/ROMsOther/fbneo","hdd:/ROMs/../outside","usb:/ROMs/fbneo","game:/ROMs/fbneo"}) {
        bad=sources();bad[1].path=path;
        assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    }
    for(const auto* command:{"shell hdd:/x.xex %ROM%","xex hdd:/x.xex", "xex hdd:/x.xex %ROM% %ROM%",
            "xex hdd:/x.xex %ROM_RAW%","xex hdd:/x.xex \"%ROM%\"", "xex hdd:/x.xex %BASENAME%", "xex hdd:/../x.xex %ROM%"}) {
        bad=sources();bad[0].command=command;
        assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    }
    for(const auto* ext:{"zip",".*",".zip .nes",".",".zip/other"}) {
        bad=sources();bad[0].extensions={ext};
        assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    }
    bad=sources();bad[0].extensions.clear();assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    bad.clear();assert(!validateXboxRoutes("hdd:/ROMs",bad,error));
    bad=sources();assert(!validateXboxRoutes("hdd:/ROMs/..",bad,error));

    // A format accepted by another core cannot cause a source to fall back.
    auto formats=sources();formats[1].extensions={".bin"};
    assert(validateXboxRoutes("hdd:/ROMs",formats,error));
    assert(!resolveXboxRoute(formats,"hdd:/ROMs/fbneo/game.bin"));
    assert(!resolveXboxRoute(formats,"hdd:/ROMs/mame2003/game.zip"));
    assert(resolveXboxRoute(formats,"hdd:/ROMs/mame2003/game.bin")==&formats[1]);

    std::string physical;
    assert(resolveXboxRouteMetadataPath("hdd:/ROMs","./fbneo/X-Men.zip",physical));
    assert(physical=="hdd:/ROMs/fbneo/X-Men.zip"); // Preserve physical filename spelling.
    assert(resolveXboxRoute(routes,physical)==&routes[0]);
    assert(makeXboxRouteRelativePath("HDD1:/ROMS/",physical)=="./fbneo/X-Men.zip");
    const auto first=xboxRoutePathKey(physical);
    assert(resolveXboxRouteMetadataPath("hdd:/ROMs",".\\mame2003\\X-Men.zip",physical));
    assert(resolveXboxRoute(routes,physical)==&routes[1]);
    assert(first!=xboxRoutePathKey(physical));
    assert(makeXboxRouteRelativePath("hdd:/ROMs",physical)=="./mame2003/X-Men.zip");
    assert(resolveXboxRouteMetadataPath("hdd:/ROMs","Hdd1:\\ROMs\\fbneo\\X-Men.zip",physical));
    assert(first==xboxRoutePathKey(physical));
    for(const auto* entry:{"../fbneo/xmen.zip","./fbneo/../mame2003/xmen.zip","fbneo/xmen.zip",
            "hdd:/ROMsOther/fbneo/xmen.zip","usb:/ROMs/fbneo/xmen.zip","././fbneo/xmen.zip"}) {
        assert(!resolveXboxRouteMetadataPath("hdd:/ROMs",entry,physical) && physical.empty());
    }
    assert(resolveXboxRouteMetadataPath("hdd:/ROMs","./unlisted/xmen.zip",physical));
    assert(!resolveXboxRoute(routes,physical)); // Metadata must satisfy root AND source membership.
    assert(makeXboxRouteRelativePath("hdd:/ROMs","hdd:/ROMsOther/xmen.zip").empty());
    std::cout<<"Xbox source boundaries, core routing, physical identity and metadata round trips passed\n";
}
