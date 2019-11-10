#pragma once

#ifdef GRINPP_STATIC
	#define EXPORT
	#define IMPORT
#else
	#if defined(_MSC_VER)
		//  Microsoft 
		#define EXPORT __declspec(dllexport)
		#define IMPORT __declspec(dllimport)
	#elif defined(__GNUC__)
		//  GCC
		#define EXPORT __attribute__((visibility("default")))
		#define IMPORT
	#else
		#define EXPORT
		#define IMPORT
	#endif
#endif