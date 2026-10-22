#pragma once
#include <string>

struct XboxLaunchCommand {
    std::string executable;
    std::string rom;
};
// A command is exactly: xex "drive:/folder/app.xex" ["drive:/folder/game.rom"]
// Paths are data; shell operators/expansion are never executed.
bool parseXboxLaunchCommand(const std::string& text,XboxLaunchCommand& out,std::string& error);
