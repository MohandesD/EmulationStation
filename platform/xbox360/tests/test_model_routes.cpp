// Integration test of the production SystemData/FileData/Gamelist model.
// Fixtures contain plain text, not ROMs. No console runtime or UI is linked.
#include "SystemData.h"
#include "Gamelist.h"
#include "Settings.h"
#include "Log.h"
#include "ThemeData.h"
#include "AudioManager.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

namespace fs=std::filesystem;

// The only presentation/environment seams. Real model, configuration, sorting,
// metadata and filesystem code is linked below; unused launch/UI code is removed
// by the linker. A mistakenly loaded common-root theme fails the test.
std::string getHomePath() { return "game:"; }
ThemeData::ThemeData():mVersion(0) {}
void ThemeData::loadFile(const std::string&) { assert(false && "Unexpected theme load in model-only test"); }
fs::path ThemeData::getThemeFromCurrentSet(const std::string& system) {
    return fs::path("game:/.emulationstation/themes/test")/system/"theme.xml";
}
// UBSan retains this presentation type's RTTI from the unused launch function.
// Supply its key function without linking or ever initializing an audio device.
AudioManager::~AudioManager() { assert(false && "Audio must not run in the model test"); }

static void write(const fs::path& path,const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path);file<<text;file.close();assert(file);
}
static std::string read(const fs::path& path) {
    std::ifstream file(path);assert(file);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
struct Fixture {
    fs::path previous=fs::current_path(),directory;
    Fixture() {
        std::string pattern=(fs::temp_directory_path()/"es-model-routes-XXXXXX").string();
        char* created=mkdtemp(pattern.data());assert(created);
        directory=created;fs::current_path(directory);
        fs::create_directories("game:/.emulationstation");Log::open();
    }
    ~Fixture() {
        SystemData::deleteSystems();Log::close();
        fs::current_path(previous);fs::remove_all(directory);
    }
};
static SystemData* findSystem(const std::string& name) {
    for(auto* item:SystemData::sSystemVector) if(item->getName()==name) return item;
    return nullptr;
}
static FileData* game(SystemData* source,const std::string& path) {
    for(auto* item:source->getRootFolder()->getChildren())
        if(item->getPath().generic_string()==path) return item;
    return nullptr;
}
static void flatGames(SystemData* source,const std::set<std::string>& expected) {
    assert(source && source->getGameCount()==expected.size());
    std::set<std::string> actual;
    for(auto* item:source->getRootFolder()->getChildren()) {
        assert(item->getType()==GAME && item->getParent()==source->getRootFolder());
        assert(item->getChildren().empty());actual.insert(item->getPath().generic_string());
    }
    assert(actual==expected);
}
static int countXmlPath(const pugi::xml_document& document,const std::string& path) {
    int count=0;
    for(auto node:document.child("gameList").children("game"))
        if(node.child("path").text().as_string()==path) count++;
    return count;
}
int main() {
    Fixture fixture;
    auto* settings=Settings::getInstance();
    settings->setBool("ParseGamelistOnly",false);settings->setBool("IgnoreGamelist",false);
    for(const auto* path:{"hdd:/ROMs/fbneo/Shared.zip","hdd:/ROMs/fbneo/nested/Deeper.zip","hdd:/ROMs/fbneo/ScanOnly.zip",
            "hdd:/ROMs/mame2003/Shared.zip","hdd:/ROMs/unlisted/Excluded.zip",
            "hdd:/ROMs/fbneo/Wrong.nes","hdd:/ROMs/fbneoplus/Excluded.zip",
            "hdd:/ROMs/ngp/Shared.ngp","hdd:/ROMs/neopop/Shared.ngp","hdd:/ROMs/nes/Tetris.nes"})
        write(path,"not a ROM\n");
    fs::create_directories("hdd:/ROMs/fbneo/NotAGame.zip");
    fs::create_directory_symlink(fs::absolute("hdd:/ROMs/unlisted"),"hdd:/ROMs/fbneo/link");
    const std::string commonGamelist=R"(<gameList><game><path>./fbneo/Shared.zip</path><name>WRONG COMMON METADATA</name></game></gameList>)";
    write("hdd:/ROMs/gamelist.xml",commonGamelist);
    write("hdd:/ROMs/theme.xml","WRONG COMMON THEME");
    write("game:/.emulationstation/es_systems.cfg",R"(<systemList>
      <system><name>arcade</name><fullname>Arcade</fullname><path>hdd:/ROMs</path><extension>.zip .ZIP</extension><command>salvia fbn "hdd:/Salvia/fbn.xex" %ROM%</command><platform>arcade</platform><theme>arcade</theme>
        <xboxRoutes>
          <route><path>hdd:/ROMs/fbneo</path><extension>.zip .ZIP</extension><command>salvia fbn "hdd:/Salvia/fbn.xex" %ROM%</command></route>
          <route><path>hdd:/ROMs/mame2003</path><extension>.zip</extension><command>salvia mame "hdd:/Salvia/mame.xex" %ROM%</command></route>
        </xboxRoutes>
      </system>
      <system><name>ngp</name><fullname>Neo Geo Pocket</fullname><path>hdd:/ROMs</path><extension>.ngp</extension><command>salvia ngp "hdd:/Salvia/ngp.xex" %ROM%</command><platform>ngp</platform><theme>ngp</theme>
        <xboxRoutes>
          <route><path>hdd:/ROMs/ngp</path><extension>.ngp</extension><command>salvia ngp "hdd:/Salvia/ngp.xex" %ROM%</command></route>
          <route><path>hdd:/ROMs/neopop</path><extension>.ngp</extension><command>salvia neopop "hdd:/Salvia/neopop.xex" %ROM%</command></route>
        </xboxRoutes>
      </system>
      <system><name>nes</name><fullname>NES</fullname><path>hdd:/ROMs/nes</path><extension>.nes</extension><command>salvia nes "hdd:/Salvia/nes.xex" %ROM%</command><platform>nes</platform></system>
      <system><name>bad_overlap</name><fullname>Bad overlap</fullname><path>hdd:/ROMs</path><extension>.zip</extension><command>salvia fbn "hdd:/Salvia/fbn.xex" %ROM%</command>
        <xboxRoutes>
          <route><path>hdd:/ROMs/fbneo</path><extension>.zip</extension><command>salvia fbn "hdd:/Salvia/fbn.xex" %ROM%</command></route>
          <route><path>hdd:/ROMs/fbneo/nested</path><extension>.zip</extension><command>salvia mame "hdd:/Salvia/mame.xex" %ROM%</command></route>
        </xboxRoutes>
      </system>
    </systemList>)");
    write("game:/.emulationstation/gamelists/arcade/gamelist.xml",R"(<gameList>
      <game><path>./fbneo/Shared.zip</path><name>FBN Shared</name><desc>First source</desc></game>
      <game><path>./mame2003/Shared.zip</path><name>MAME Shared</name><desc>Second source</desc></game>
      <game><path>./fbneo/nested/Deeper.zip</path><name>Nested game</name></game>
      <game><path>./unlisted/Excluded.zip</path><name>Outside sources</name></game>
      <game><path>./fbneo/Wrong.nes</path><name>Wrong extension</name></game>
      <game><path>./fbneo/../mame2003/Shared.zip</path><name>Traversal must not replace metadata</name></game>
      <game><path>./fbneo/Missing.zip</path><name>Missing file</name></game>
      <folder><path>./fbneo</path><name>No backend folder</name></folder>
    </gameList>)");
    write("game:/.emulationstation/gamelists/ngp/gamelist.xml",R"(<gameList>
      <game><path>./ngp/Shared.ngp</path><name>NGP Shared</name></game>
      <game><path>./neopop/Shared.ngp</path><name>NeoPop Shared</name></game>
    </gameList>)");
    write("hdd:/ROMs/nes/gamelist.xml",R"(<gameList><game><path>./Tetris.nes</path><name>Original NES metadata</name></game></gameList>)");

    const std::set<std::string> arcadeMetadataFiles={"hdd:/ROMs/fbneo/Shared.zip","hdd:/ROMs/mame2003/Shared.zip","hdd:/ROMs/fbneo/nested/Deeper.zip"};
    auto arcadeFiles=arcadeMetadataFiles;arcadeFiles.insert("hdd:/ROMs/fbneo/ScanOnly.zip");
    const std::set<std::string> ngpFiles={"hdd:/ROMs/ngp/Shared.ngp","hdd:/ROMs/neopop/Shared.ngp"};
    // Keep scanning independent from metadata: otherwise findOrCreateFile could
    // hide a missing or incorrectly wired populateXboxRoutes call.
    settings->setBool("IgnoreGamelist",true);
    assert(SystemData::loadConfig());
    flatGames(findSystem("arcade"),arcadeFiles);flatGames(findSystem("ngp"),ngpFiles);
    assert(game(findSystem("arcade"),"hdd:/ROMs/fbneo/Shared.zip")->getName()=="Shared");
    assert(game(findSystem("arcade"),"hdd:/ROMs/mame2003/Shared.zip")->getName()=="Shared");
    SystemData::deleteSystems(); // IgnoreGamelist still suppresses writes here.
    settings->setBool("IgnoreGamelist",false);

    assert(SystemData::loadConfig());
    assert(SystemData::sSystemVector.size()==3 && !findSystem("bad_overlap"));
    auto* arcade=findSystem("arcade");auto* ngp=findSystem("ngp");auto* nes=findSystem("nes");
    flatGames(arcade,arcadeFiles);flatGames(ngp,ngpFiles);
    assert(game(arcade,"hdd:/ROMs/fbneo/Shared.zip")->getName()=="FBN Shared");
    assert(game(arcade,"hdd:/ROMs/mame2003/Shared.zip")->getName()=="MAME Shared");
    assert(game(arcade,"hdd:/ROMs/fbneo/Shared.zip")->metadata.get("desc")=="First source");
    assert(game(arcade,"hdd:/ROMs/mame2003/Shared.zip")->metadata.get("desc")=="Second source");
    assert(arcade->getXboxRoute("hdd:/ROMs/fbneo/Shared.zip")->command.find("salvia fbn ")==0);
    assert(arcade->getXboxRoute("hdd:/ROMs/mame2003/Shared.zip")->command.find("salvia mame ")==0);
    assert(arcade->getGamelistPath(false)=="game:/.emulationstation/gamelists/arcade/gamelist.xml");
    assert(ngp->getGamelistPath(false)=="game:/.emulationstation/gamelists/ngp/gamelist.xml");
    assert(arcade->getThemePath()=="game:/.emulationstation/themes/test/arcade/theme.xml");
    assert(ngp->getThemePath()=="game:/.emulationstation/themes/test/ngp/theme.xml");
    assert(!nes->hasXboxRoutes() && nes->getGamelistPath(false)=="hdd:/ROMs/nes/gamelist.xml");
    assert(game(nes,"hdd:/ROMs/nes/Tetris.nes")->getName()=="Original NES metadata");

    // Change an existing XML entry to an equivalent Xbox drive/case spelling.
    // updateGamelist must replace it, not append a second same-file record.
    pugi::xml_document before;
    assert(before.load_file(arcade->getGamelistPath(false).c_str()));
    for(auto node:before.child("gameList").children("game"))
        if(std::string(node.child("path").text().get())=="./mame2003/Shared.zip")
            node.child("path").text().set("HDD1:\\ROMS\\MAME2003\\Shared.zip");
    assert(before.save_file(arcade->getGamelistPath(true).c_str()));
    game(arcade,"hdd:/ROMs/fbneo/Shared.zip")->metadata.set("desc","Updated first source");
    game(arcade,"hdd:/ROMs/mame2003/Shared.zip")->metadata.set("desc","Updated second source");
    updateGamelist(arcade);updateGamelist(ngp);
    pugi::xml_document saved;
    assert(saved.load_file(arcade->getGamelistPath(false).c_str()));
    assert(countXmlPath(saved,"./fbneo/Shared.zip")==1);
    assert(countXmlPath(saved,"./mame2003/Shared.zip")==1);
    assert(countXmlPath(saved,"./fbneo/ScanOnly.zip")==0); // Default-only entries are not saved.
    assert(countXmlPath(saved,"HDD1:\\ROMS\\MAME2003\\Shared.zip")==0);
    assert(read("hdd:/ROMs/gamelist.xml")==commonGamelist);

    assert(SystemData::loadConfig());
    arcade=findSystem("arcade");ngp=findSystem("ngp");
    flatGames(arcade,arcadeFiles);flatGames(ngp,ngpFiles);
    assert(game(arcade,"hdd:/ROMs/fbneo/Shared.zip")->metadata.get("desc")=="Updated first source");
    assert(game(arcade,"hdd:/ROMs/mame2003/Shared.zip")->metadata.get("desc")=="Updated second source");

    // Actual parse-only constructor path: valid metadata recreates flat entries
    // and excludes traversal, folders, missing files and wrong-source formats.
    // The existing ScanOnly.zip file must now disappear, proving no scan ran.
    settings->setBool("ParseGamelistOnly",true);
    assert(SystemData::loadConfig());
    arcade=findSystem("arcade");ngp=findSystem("ngp");
    flatGames(arcade,arcadeMetadataFiles);flatGames(ngp,ngpFiles);
    assert(game(arcade,"hdd:/ROMs/fbneo/Shared.zip")->getName()=="FBN Shared");
    assert(game(arcade,"hdd:/ROMs/mame2003/Shared.zip")->getName()=="MAME Shared");
    assert(read("hdd:/ROMs/gamelist.xml")==commonGamelist);
    std::cout<<"Production XML load, routed scan, flattened model, metadata identity and save/reload checks passed\n";
}
