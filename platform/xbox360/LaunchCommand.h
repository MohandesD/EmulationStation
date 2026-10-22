#pragma once
#include <string>

struct XboxLaunchCommand {
    std::string executable;
    std::string rom;
    std::string salviaSystem;
};
// A command is exactly: xex "drive:/folder/app.xex" ["drive:/folder/game.rom"]
// Or: salvia system_cfg_name "drive:/Salvia/core.xex" "drive:/game.rom"
// Paths are data; shell operators/expansion are never executed.
bool parseXboxLaunchCommand(const std::string& text,XboxLaunchCommand& out,std::string& error);
// Check a parsed command before dismantling the frontend. No title is launched.
bool validateXboxLaunchFiles(const XboxLaunchCommand& command,std::string& error);
// A single explicit ROM can be tested while the general launch gate stays off.
bool isXboxLaunchPermitted(const XboxLaunchCommand& command,bool enabled,const std::string& testRom);
// A successful handoff replaces this process. Any returned call provides the
// launch failure, or the more urgent session-restoration failure, for the UI.
int runXboxSystemCommand(const std::string& text,std::string& error);
// Fixed diagnostic receiver and text fixture only; does not enable game launches.
int runXboxHandoffDiagnostic();
