#include <ipasir2cpp.h>

#include "custom_types.h"
#include "ipasir2_mock_factory.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>


#include <vector>

#if __has_include(<version>)
#include <version>
#endif


namespace ip2 = ipasir2;

using clause_vec = std::vector<std::vector<int32_t>>;

TEST_CASE("solver::set_export_callback()")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();

  SECTION("Successfully set and clear export callback")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    std::vector<clause_vec> received_clauses{3};

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_OK});
    solver->set_export_callback(
        [&](ip2::clause_view<int32_t> clause) {
          received_clauses[0].emplace_back(clause.begin(), clause.end());
        },
        1024);
    mock->simulate_export_callback_call(1, {-1, 2, 0});
    CHECK(received_clauses == std::vector<clause_vec>{{{-1, 2}}, {}, {}});


    mock->expect_call(1, set_export_call{true, 512, IPASIR2_E_OK});
    solver->set_export_callback(
        [&](ip2::clause_view<int32_t> clause) {
          received_clauses[1].emplace_back(clause.begin(), clause.end());
        },
        512);
    mock->simulate_export_callback_call(1, {0});
    CHECK(received_clauses == std::vector<clause_vec>{{{-1, 2}}, {{}}, {}});


    mock->expect_call(1, set_export_call{false, 0, IPASIR2_E_OK});
    solver->clear_export_callback();


    mock->expect_call(1, set_export_call{true, 0, IPASIR2_E_OK});
    solver->set_export_callback([&](ip2::clause_view<int32_t> clause) {
      received_clauses[2].emplace_back(clause.begin(), clause.end());
    });
    mock->simulate_export_callback_call(1, {3, 5, 0});
    CHECK(received_clauses == std::vector<clause_vec>{{{-1, 2}}, {{}}, {{3, 5}}});
  }


  SECTION("When set_export_callback() fails, exception is thrown and old callback is not called "
          "anymore")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    bool called_old_callback = false;

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_OK});
    solver->set_export_callback([&](ip2::clause_view<int32_t>) { called_old_callback = true; },
                                1024);

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_UNKNOWN});
    CHECK_THROWS_AS(solver->set_export_callback([&](ip2::clause_view<int32_t>) {}, 1024),
                    ip2::ipasir2_error);

    mock->simulate_export_callback_call(1, {1, 0});
    CHECK(!called_old_callback);
  }


  SECTION("When clear_export_callback() fails, exception is thrown and old callback is not "
          "called anymore")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    bool called_old_callback = false;

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_OK});
    solver->set_export_callback([&](ip2::clause_view<int32_t>) { called_old_callback = true; },
                                1024);

    mock->expect_call(1, set_export_call{false, 0, IPASIR2_E_UNKNOWN});
    CHECK_THROWS_AS(solver->clear_export_callback(), ip2::ipasir2_error);

    mock->simulate_export_callback_call(1, {1, 0});
    CHECK(!called_old_callback);
  }


#if __cpp_lib_span
  SECTION("Use export callback with std::span")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    std::vector<int32_t> received_clause;

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_OK});
    solver->set_export_callback(
        [&](std::span<int32_t const> clause) {
          received_clause.assign(clause.begin(), clause.end());
        },
        1024);

    mock->simulate_export_callback_call(1, {1, 2, 3, 0});

    CHECK(received_clause == std::vector<int32_t>{1, 2, 3});
  }
#endif


  SECTION("Use export callback with custom literal type")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    using custom_lit_test::lit;

    std::vector<lit> received_clause;

    mock->expect_call(1, set_export_call{true, 1024, IPASIR2_E_OK});
    solver->set_export_callback<lit>(
        [&](ip2::clause_view<lit> clause) { received_clause.assign(clause.begin(), clause.end()); },
        1024);

    mock->simulate_export_callback_call(1, {1, 2, 3, 0});
    CHECK(received_clause == std::vector<lit>{lit{1, true}, lit{2, true}, lit{3, true}});

    mock->simulate_export_callback_call(1, {0});
    CHECK(received_clause.empty());

    mock->simulate_export_callback_call(1, {2, 3, 0});
    CHECK(received_clause == std::vector<lit>{lit{2, true}, lit{3, true}});
  }


  SECTION("When an exception is thrown in the callback, it is rethrown once from solve()")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    mock->expect_call(1, set_export_call{true, 0, IPASIR2_E_OK});
    solver->set_export_callback([&](auto const&) { throw std::runtime_error{"test exception"}; });

    // Caveat: this test relies on an implementation detail: callbacks are called during solve(),
    // but here the callback is simulated before the actual solve() call. This keeps some
    // complexity out of the mocking system. If the solver would check for exceptions before
    // invoking ipasir2_solve(), the test would fail though, since that call would not be observed.

    mock->simulate_export_callback_call(1, {1, 2, 3, 0});

    SECTION("Check solve() overload without assumptions")
    {
      mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
      CHECK_THROWS_MATCHES(
          solver->solve(), std::runtime_error, Catch::Matchers::Message("test exception"));

      mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
      CHECK(solver->solve() == ip2::optional_bool{true});
    }

    SECTION("Check solve() overload with assumptions")
    {
      std::vector<int32_t> assumptions = {1, 2};

      mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
      CHECK_THROWS_MATCHES(solver->solve(assumptions),
                           std::runtime_error,
                           Catch::Matchers::Message("test exception"));

      mock->expect_call(1, solve_call{assumptions, 10, IPASIR2_E_OK});
      CHECK(solver->solve(assumptions) == ip2::optional_bool{true});
    }
  }


  CHECK(!mock->has_outstanding_expects());
}
