#pragma once

#include <stdint.h> // for types uint32_t,uint64_t
#include <chrono>
#include <ctime>

#pragma warning(push)
#pragma warning(disable:4804) // unsafe use of type 'bool' in operation
#include "crypto/blake2.h"
#pragma warning(pop)

#include "crypto/siphash.hpp"

// save some keystrokes since i'm a lazy typer
typedef uint32_t u32;
typedef uint64_t u64;

enum verify_code { POW_OK, POW_HEADER_LENGTH, POW_TOO_BIG, POW_TOO_SMALL, POW_NON_MATCHING, POW_BRANCH, POW_DEAD_END, POW_SHORT_CYCLE, POW_UNBALANCED};
static const char* errstr[] = { "OK", "wrong header length", "edge too big", "edges not ascending", "endpoints don't match up", "branch in cycle", "cycle dead ends", "cycle too short", "edges not balanced" };

// convenience function for extracting siphash keys from header
static void setheader(const char *header, const u32 headerlen, siphash_keys *keys) {
	char hdrkey[32];
	// SHA256((unsigned char *)header, headerlen, (unsigned char *)hdrkey);
	blake2b((void *)hdrkey, sizeof(hdrkey), (const void *)header, headerlen, 0, 0);
	keys->setkeys(hdrkey);
}

static u64 timestamp() {
	using namespace std::chrono;
	high_resolution_clock::time_point now = high_resolution_clock::now();
	auto dn = now.time_since_epoch();
	return dn.count();
}

/////////////////////////////////////////////////////////////////
// Declarations to make it easier for callers to link as required
/////////////////////////////////////////////////////////////////

#ifndef C_CALL_CONVENTION
#define C_CALL_CONVENTION 0
#endif

// convention to prepend to called functions
#if C_CALL_CONVENTION
#define CALL_CONVENTION extern "C"
#else
#define CALL_CONVENTION
#endif

// Ability to squash printf output at compile time, if desired
#ifndef SQUASH_OUTPUT
#define SQUASH_OUTPUT 0
#endif

static void print_log(const char *fmt, ...) {
	if (SQUASH_OUTPUT) return;
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
//////////////////////////////////////////////////////////////////
// END caller QOL
//////////////////////////////////////////////////////////////////