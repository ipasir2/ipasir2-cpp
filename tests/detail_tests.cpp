#include <ipasir2cpp.h>

#include <list>
#include <vector>

namespace ip2 = ipasir2;

// In C++20, concepts are used to detect contiguous iterators.
// For C++17, this is not possible in general, but std::vector
// iterators and pointers to clause buffers are still recognized.
#if !defined(__cpp_lib_concepts)
static_assert(ip2::detail::is_contiguous_lit_iter<std::vector<int32_t>::iterator>);
static_assert(ip2::detail::is_contiguous_lit_iter<std::vector<int32_t>::const_iterator>);
static_assert(ip2::detail::is_contiguous_lit_iter<int32_t*>);
static_assert(ip2::detail::is_contiguous_lit_iter<int32_t const*>);
static_assert(!ip2::detail::is_contiguous_lit_iter<std::list<int32_t>::iterator>);
#endif
