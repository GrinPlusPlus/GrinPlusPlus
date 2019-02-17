#pragma once

#if defined(_MSC_VER)
//  Microsoft 
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
// TODO: Determine how to handle this
#define EXPORT
#define IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif