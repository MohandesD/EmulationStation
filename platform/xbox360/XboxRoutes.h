#pragma once
#include <string>
#include <vector>

struct XboxRoute {
    std::string path;
    std::vector<std::string> extensions;
    std::string command;
};

// Validate disjoint source folders strictly beneath the collection root and
// each launch template with the existing native launch-command parser.
// On success paths/extensions are normalized; on failure routes are unchanged.
bool validateXboxRoutes(const std::string& root,std::vector<XboxRoute>& routes,std::string& error);
// routes must have passed validation. Match full physical path and extension;
// never fall back to another route merely because its emulator accepts a file.
const XboxRoute* resolveXboxRoute(const std::vector<XboxRoute>& routes,const std::string& path);
// Full physical identity under the Xbox's case-insensitive drive semantics.
// Invalid paths, relative paths and traversal yield an empty key.
std::string xboxRoutePathKey(const std::string& path);
bool resolveXboxRouteMetadataPath(const std::string& root,const std::string& entry,std::string& path);
std::string makeXboxRouteRelativePath(const std::string& root,const std::string& path);
