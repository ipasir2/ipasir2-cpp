#include <ipasir2cpp.h>

#include "custom_types.h"
#include "ipasir2_mock_factory.h"

#include <list>
#include <vector>

#if __has_include(<version>)
#include <version>
#endif

#if __has_include(<span>)
#include <span>
#endif

#include <catch2/catch_test_macros.hpp>

namespace ip2 = ipasir2;
using ip2::optional_bool;

TEST_CASE("solver::solve() functions")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();
  std::vector<int32_t> assumptions = {1, -2, 3};


  SECTION("Successfully solve without assumptions")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{{}, 0, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{}, 20, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->solve() == optional_bool{});
    CHECK(solver->solve() == optional_bool{true});
    CHECK(solver->solve() == optional_bool{false});
  }


  SECTION("Successfully add clauses via parameter-pack function")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{{1}, 0, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, -2}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, -2, 3}, 20, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, -2, 3, -4}, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->solve(1) == optional_bool{});
    CHECK(solver->solve(1, -2) == optional_bool{true});
    CHECK(solver->solve(1, -2, 3) == optional_bool{false});
    CHECK(solver->solve(1, -2, 3, -4) == optional_bool{});
  }


  SECTION("Successfully solve with assumptions vector")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 0, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{assumptions, 20, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->solve(assumptions) == optional_bool{});
    CHECK(solver->solve(assumptions) == optional_bool{true});
    CHECK(solver->solve(assumptions) == optional_bool{false});
  }


  SECTION("Successfully solve with non-vector assumption iterables")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    std::list<int32_t> assumptions_list{assumptions.begin(), assumptions.end()};
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    CHECK(solver->solve(assumptions_list) == optional_bool{true});

#if __cpp_lib_span
    std::span<int32_t> assumptions_span = assumptions;
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
    CHECK(solver->solve(assumptions_span) == optional_bool{true});
#endif
  }


  SECTION("Successfully solve with custom assumption-container types")
  {
    mock->expect_init_call(1);

    mock->expect_call(1, solve_call{{1, 2}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, 3}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, 4}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, 5}, 10, IPASIR2_E_OK});

    custom_lit_container_1 assum1{{1, 2}};
    custom_lit_container_1 const assum2{{1, 3}};
    adl_test::custom_lit_container_2 assum3{{1, 4}};
    adl_test::custom_lit_container_2 const assum4{{1, 5}};

    auto solver = api.create_solver();
    CHECK(solver->solve(assum1) == optional_bool{true});
    CHECK(solver->solve(assum2) == optional_bool{true});
    CHECK(solver->solve(assum3) == optional_bool{true});
    CHECK(solver->solve(assum4) == optional_bool{true});
  }


  SECTION("Successfully solve with custom assumption literal type")
  {
    mock->expect_init_call(1);

    mock->expect_call(1, solve_call{{1}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, 2}, 10, IPASIR2_E_OK});
    mock->expect_call(1, solve_call{{1, 2, 3}, 10, IPASIR2_E_OK});

    using custom_lit_test::lit;

    auto solver = api.create_solver();
    CHECK(solver->solve(lit{1, true}) == optional_bool{true});
    CHECK(solver->solve(lit{1, true}, lit{2, true}) == optional_bool{true});
    CHECK(solver->solve(lit{1, true}, lit{2, true}, lit{3, true}) == optional_bool{true});
  }


  SECTION("Successfully solve with contiguous assumption iterators")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->solve(assumptions.begin(), assumptions.end()) == optional_bool{true});
  }


  SECTION("Successfully solve with non-contiguous assumption iterators")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});

    auto solver = api.create_solver();
    std::list<int32_t> assumptions_list{assumptions.begin(), assumptions.end()};
    CHECK(solver->solve(assumptions_list.begin(), assumptions_list.end()) == optional_bool{true});
  }


  SECTION("Throws when ipasir2_solve() returns error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_UNKNOWN});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_UNKNOWN});
    mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_UNKNOWN});
    mock->expect_call(1, solve_call{{1}, 10, IPASIR2_E_UNKNOWN});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->solve(), ip2::ipasir2_error);
    CHECK_THROWS_AS(solver->solve(assumptions), ip2::ipasir2_error);
    CHECK_THROWS_AS(solver->solve(assumptions.begin(), assumptions.end()), ip2::ipasir2_error);
    CHECK_THROWS_AS(solver->solve(1), ip2::ipasir2_error);
  }

  CHECK(!mock->has_outstanding_expects());
}
