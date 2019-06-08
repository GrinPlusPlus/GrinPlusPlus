#pragma once

 #if defined(__cplusplus) && defined(__has_include) && __has_include(<filesystem>)
 #include <filesystem>
 namespace fs = std::filesystem;
 #else
 #include <ghc/filesystem.hpp>
 namespace fs = ghc::filesystem;
 #endif