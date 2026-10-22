#pragma once
#include <string>

struct XboxLaunchCommand {
    std::string executable;
    std::string rom;
};
// A command is exactly: xex "drive:/folder/app.xex" ["drive:/folder/game.rom"]
// Paths are data; shell operators/expansion are never executed.
bool parseXboxLaunchCommand(const std::string& text,XboxLaunchCommand& out,std::string& error);
// Check a parsed command before dismantling the frontend. No title is launched.
bool validateXboxLaunchFiles(const XboxLaunchCommand& command,std::string& error);
// Fixed diagnostic receiver and text fixture only; does not enable game launches.
int runXboxHandoffDiagnostic();
