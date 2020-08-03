#include <Common/Secure.h>

#ifndef __STDC_WANT_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#endif
#include <string.h>
#include <Common/Compat.h>

#ifndef _WIN32
#include <sys/mman.h>
#endif

void SecureMem::LockMemory(void* pMemory, size_t size)
{
#if defined(_MSC_VER)
	VirtualLock(pMemory, size);
#else
	mlock(pMemory, size);
#endif
}

void SecureMem::UnlockMemory(void* pMemory, size_t size)
{
#if defined(_MSC_VER)
	VirtualUnlock(pMemory, size);
#else
	munlock(pMemory, size);
#endif
}

/* Compilers have a bad habit of removing "superfluous" memset calls that
 * are trying to zero memory. For example, when memset()ing a buffer and
 * then free()ing it, the compiler might decide that the memset is
 * unobservable and thus can be removed.
 *
 * Previously we used OpenSSL which tried to stop this by a) implementing
 * memset in assembly on x86 and b) putting the function in its own file
 * for other platforms.
 *
 * This change removes those tricks in favour of using asm directives to
 * scare the compiler away. As best as our compiler folks can tell, this is
 * sufficient and will continue to be so.
 *
 * Adam Langley <agl@google.com>
 * Commit: ad1907fe73334d6c696c8539646c21b11178f20f
 * BoringSSL (LICENSE: ISC)
 */
void SecureMem::Cleanse(void* ptr, size_t len)
{
#ifdef HAS_MEMSET_S
	memset_s(ptr, len, 0, len);
#elif defined(_MSC_VER)
	SecureZeroMemory(ptr, len);
#else
	std::memset(ptr, 0, len);
	__asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
}