#pragma once

#if defined(__APPLE__)
#undef _GLIBCXX_HAVE_TIMESPEC_GET
#endif
/* https://github.com/hboetes/mg/issues/7#issuecomment-475869095 */
#if defined(__APPLE__) || defined(__NetBSD__)
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

 #if defined(__cplusplus) && defined(__has_include) && __has_include(<filesystem>) && !defined(__APPLE__)
 #include <filesystem>
 namespace fs = std::filesystem;
 #else
 #include "filesystem.hpp"
 namespace fs = ghc::filesystem;
 #endif