#pragma once

#include <string>
#include <vector>
#include <cstring>

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static void LOCK_MEMORY(void* pMemory, size_t size)
{
#if defined(_MSC_VER)
	VirtualLock(pMemory, size);
#else
// TODO: 
#endif
}

static void UNLOCK_MEMORY(void* pMemory, size_t size)
{
#if defined(_MSC_VER)
	VirtualUnlock(pMemory, size);
#else
// TODO: 
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
static void cleanse(void *ptr, size_t len)
{
	std::memset(ptr, 0, len);

	/* As best as we can tell, this is sufficient to break any optimisations that
	   might try to eliminate "superfluous" memsets. If there's an easy way to
	   detect memset_s, it would be better to use that. */
#if defined(_MSC_VER)
	SecureZeroMemory(ptr, len);
#else
	__asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
}

//
// Allocator that clears its contents before deletion
//
template<typename T>
struct secure_allocator : public std::allocator<T>
{
	// MSVC8 default copy constructor is broken
	typedef std::allocator<T> base;
	typedef typename base::size_type size_type;
	typedef typename base::difference_type  difference_type;
	typedef typename base::pointer pointer;
	typedef typename base::const_pointer const_pointer;
	typedef typename base::reference reference;
	typedef typename base::const_reference const_reference;
	typedef typename base::value_type value_type;

	constexpr secure_allocator() noexcept {}
	constexpr secure_allocator(const secure_allocator& a) noexcept : base(a) {}
	template <typename U>
	constexpr secure_allocator(const secure_allocator<U>& a) noexcept : base(a) {}
	~secure_allocator() noexcept {}

	template<typename _Other> struct rebind
	{
		typedef secure_allocator<_Other> other;
	};

	T* allocate(std::size_t n, const void* hint = 0)
	{
		T* p = std::allocator<T>::allocate(n, hint);
		if (p != NULL)
		{
			LOCK_MEMORY(p, sizeof(T) * n);
		}

		return p;
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		if (p != NULL)
		{
			cleanse(p, sizeof(T) * n);
			UNLOCK_MEMORY(p, sizeof(T) * n);
		}
		std::allocator<T>::deallocate(p, n);
	}
};

typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char>> SecureString;
typedef std::vector<unsigned char, secure_allocator<unsigned char>> SecureVector;
