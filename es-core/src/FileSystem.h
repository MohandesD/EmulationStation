#pragma once
#ifdef __XSDK__
#include <filesystem>
// Keep the upstream model's types intact at call sites while using the SDK's
// tested C++ filesystem implementation. No host Boost filesystem binary is linked.
namespace boost { namespace filesystem = std::filesystem; }
#else
#include <boost/filesystem.hpp>
#endif
