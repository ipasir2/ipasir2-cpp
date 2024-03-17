#include <ipasir2cpp.h>

#include "mock/ipasir2_mock.h"

#include <list>
#include <vector>

#if __has_include(<version>)
#include <version>
#endif

#if __has_include(<span>)
#include <span>
#endif

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("solver::solve() functions")
{
  auto mock = create_ipasir2_mock();
  ip2::ipasir2 api;
  std::vector<int32_t> assumptions = {1, -2, 3};

  using ip2::optional_bool;


  SUBCASE("Successfully solve without assumptions")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{{}, 0, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{}, 20, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_EQ(solver->solve(), optional_bool{});
    CHECK_EQ(solver->solve(), optional_bool{true});
    CHECK_EQ(solver->solve(), optional_bool{false});
  }


  SUBCASE("Successfully solve with assumptions vector")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 0, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{assumptions, 20, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_EQ(solver->solve(assumptions), optional_bool{});
    CHECK_EQ(solver->solve(assumptions), optional_bool{true});
    CHECK_EQ(solver->solve(assumptions), optional_bool{false});
  }


  SUBCASE("Successfully solve with non-vector assumption containers")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    std::list<int32_t> assumptions_list{assumptions.begin(), assumptions.end()};
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    CHECK_EQ(solver->solve(assumptions_list), optional_bool{true});

#if __cpp_lib_span
    std::span<int32_t> assumptions_span = assumptions;
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    CHECK_EQ(solver->solve(assumptions_span), optional_bool{true});
#endif
  }


  SUBCASE("Successfully solve with contiguous assumption iterators")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_EQ(solver->solve(assumptions.begin(), assumptions.end()), optional_bool{true});
  }


  SUBCASE("Successfully solve with non-contiguous assumption iterators")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});

    auto solver = api.create_solver();
    std::list<int32_t> assumptions_list{assumptions.begin(), assumptions.end()};
    CHECK_EQ(solver->solve(assumptions_list.begin(), assumptions_list.end()), optional_bool{true});
  }


  SUBCASE("Throws when ipasir2_solve() returns error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_UNKNOWN});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_UNKNOWN});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_UNKNOWN});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->solve(), ip2::ipasir2_error const&);
    CHECK_THROWS_AS(solver->solve(assumptions), ip2::ipasir2_error const&);
    CHECK_THROWS_AS(solver->solve(assumptions.begin(), assumptions.end()),
                    ip2::ipasir2_error const&);
  }

  CHECK(!mock->has_outstanding_expects());
}
