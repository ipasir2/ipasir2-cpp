#include <ipasir2cpp.h>

#include "custom_types.h"

#include <list>
#include <vector>

#include <doctest.h>


namespace ip2 = ipasir2;

// In C++20, concepts are used to detect contiguous iterators.
// For C++17, this is not possible in general, but std::vector
// iterators and pointers to clause buffers are still recognized.
#if !defined(__cpp_lib_concepts)
static_assert(ip2::detail::is_contiguous_int32_iter<std::vector<int32_t>::iterator>);
static_assert(ip2::detail::is_contiguous_int32_iter<std::vector<int32_t>::const_iterator>);
static_assert(ip2::detail::is_contiguous_int32_iter<int32_t*>);
static_assert(ip2::detail::is_contiguous_int32_iter<int32_t const*>);
static_assert(!ip2::detail::is_contiguous_int32_iter<std::list<int32_t>::iterator>);
#endif


static_assert(ip2::detail::is_literal_v<int32_t>);
static_assert(ip2::detail::is_literal_v<int32_t const>);
static_assert(!ip2::detail::is_literal_v<int16_t>);
static_assert(!ip2::detail::is_literal_v<int16_t const>);
static_assert(ip2::detail::is_literal_v<custom_lit_test::lit>);
static_assert(ip2::detail::is_literal_v<custom_lit_test::lit const>);


TEST_CASE("detail::as_contiguous_int32s()")
{
  SUBCASE("For std::vector<int32_t>, the buffer is directly returned")
  {
    std::vector<int32_t> input = {1, 2, 3};
    std::vector<int32_t> buffer;

    auto const& [lit_ptr, size]
        = ip2::detail::as_contiguous_int32s(input.begin(), input.end(), buffer);
    CHECK_EQ(lit_ptr, input.data());
    CHECK_EQ(size, 3);
  }


  SUBCASE("For std::list<int32_t>, the literals are copied")
  {
    std::list<int32_t> input = {1, -2, 3};
    std::vector<int32_t> buffer;

    auto const& [lit_ptr, size]
        = ip2::detail::as_contiguous_int32s(input.begin(), input.end(), buffer);

    CHECK_EQ(lit_ptr, buffer.data());
    CHECK_EQ(size, 3);
    CHECK_EQ(buffer, std::vector{1, -2, 3});
  }


  SUBCASE("For std::vector<custom_lit_test::lit>, the literals are copied")
  {
    using custom_lit_test::lit;
    std::vector<lit> input = {lit{1, true}, lit{2, false}, lit{3, true}};
    std::vector<int32_t> buffer;

    auto const& [lit_ptr, size]
        = ip2::detail::as_contiguous_int32s(input.begin(), input.end(), buffer);

    CHECK_EQ(lit_ptr, buffer.data());
    CHECK_EQ(size, 3);
    CHECK_EQ(buffer, std::vector{1, -2, 3});
  }


  SUBCASE("For empty inputs, a size-zero buffer is returned")
  {
    std::vector<int32_t> input;
    std::vector<int32_t> buffer;

    auto const& [lit_ptr, size]
        = ip2::detail::as_contiguous_int32s(input.begin(), input.end(), buffer);
    CHECK_EQ(lit_ptr, buffer.data());
    CHECK_EQ(size, 0);
    CHECK_EQ(buffer.size(), 1);
  }
}
