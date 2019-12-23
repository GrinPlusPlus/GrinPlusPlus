#pragma once

#include <ctime>

#if defined(__APPLE__)
#undef _GLIBCXX_HAVE_ALIGNED_ALLOC
#endif

#pragma warning(push)
#pragma warning(disable:4018)
#pragma warning(disable:4127)
#pragma warning(disable:4211)
#pragma warning(disable:4244)
#include <CRoaring/roaring.hh>
#pragma warning(pop)