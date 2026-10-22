// C++ runtime / filesystem diagnostic. No game launch or boot configuration writes.
#include <xsdk/log.h>
#include <xsdk/xam.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    bool passed=false;
    try {
        namespace fs=std::filesystem;
        const fs::path directory="game:/cpp-probe";
        fs::create_directories(directory);
        const fs::path file=directory/"runtime.txt";
        const std::string expected="EmulationStation C++ filesystem test";
        { std::ofstream output(file); output<<expected; if(!output) throw std::runtime_error("write failed"); }
        std::string text; { std::ifstream input(file); std::getline(input,text); }
        if(text!=expected || !fs::is_regular_file(file)) throw std::runtime_error("readback failed");
        const auto canonical=fs::canonical(file);
        if(!fs::exists(canonical)) throw std::runtime_error("canonical path failed");
        size_t count=0; for(const auto& entry:fs::directory_iterator(directory)) if(entry.path().filename()=="runtime.txt") ++count;
        if(count!=1) throw std::runtime_error("directory enumeration failed");
        std::vector<std::string> values={"Xbox","360"};
        std::ostringstream formatted; formatted<<values[0]<<' '<<values[1];
        if(formatted.str()!="Xbox 360") throw std::runtime_error("C++ stream failed");
        bool caught=false;
        try { throw std::runtime_error("expected exception"); } catch(const std::exception&) { caught=true; }
        if(!caught) throw std::runtime_error("exception handling failed");
        passed=true;
    } catch(const std::exception& error) {
        xsdk_log_write("ES C++ diagnostic failed: %s",error.what());
    }
    xsdk_log_write("ES C++ runtime/filesystem diagnostic: %s",passed ? "PASS" : "FAIL");
    return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",nullptr,0);
}
