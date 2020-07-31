#pragma once

#include <string>
#include <vector>
#include <cstring>

class SecureMem
{
public:
	static void LockMemory(void* pMemory, size_t size);
	static void UnlockMemory(void* pMemory, size_t size);
	static void Cleanse(void* ptr, size_t len);
};

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
			SecureMem::LockMemory(p, sizeof(T) * n);
		}

		return p;
	}

	void deallocate(T* p, std::size_t n) noexcept
	{
		if (p != NULL)
		{
			SecureMem::Cleanse(p, sizeof(T) * n);
			SecureMem::UnlockMemory(p, sizeof(T) * n);
		}
		std::allocator<T>::deallocate(p, n);
	}
};

typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char>> SecureString;
typedef std::vector<unsigned char, secure_allocator<unsigned char>> SecureVector;
