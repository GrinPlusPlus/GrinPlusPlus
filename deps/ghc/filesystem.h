#pragma once

#if defined(__APPLE__)
#undef _GLIBCXX_HAVE_TIMESPEC_GET
#endif

 #if defined(__cplusplus) && defined(__has_include) && __has_include(<filesystem>)
 #include <filesystem>
 namespace fs = std::filesystem;
 #else
 #include "filesystem.hpp"
 namespace fs = ghc::filesystem;
 #endif