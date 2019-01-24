#pragma once

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#include <string>

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
	secure_allocator() throw() {}
	secure_allocator(const secure_allocator& a) throw() : base(a) {}
	~secure_allocator() throw() {}
	template<typename _Other> struct rebind
	{
		typedef secure_allocator<_Other> other;
	};

	void deallocate(T* p, std::size_t n)
	{
		if (p != NULL)
			memset(p, 0, sizeof(T) * n);
		std::allocator_traits<T>::deallocate(p, m_value.string, 1);
	}
};

typedef std::basic_string<char, std::char_traits<char>, secure_allocator<char>> SecureString;