#pragma once
#include "LaunchCommand.h"

// Salvia v1.0.10 restores menu.tmp's system even for an incoming ROM.
// Validate its known Xbox layout and the explicit system/core association.
bool validateSalviaLaunch(const XboxLaunchCommand&,std::string& error,bool requireReady=false);
// Preserve the old menu state and synchronize only ordinary Salvia menu state.
// Call after launch permission/preflight, before dismantling the frontend.
bool prepareSalviaLaunch(const XboxLaunchCommand&,std::string& error);
