#pragma once

 #if defined(__cplusplus) && defined(__has_include) && __has_include(<filesystem>) && !defined(__APPLE__)
 #include <filesystem>
 namespace fs = std::filesystem;
 #else
 #include "filesystem.hpp"
 namespace fs = ghc::filesystem;
 #endif