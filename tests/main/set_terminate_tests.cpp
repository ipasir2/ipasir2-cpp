#include <ipasir2cpp.h>

#include "ipasir2_mock_factory.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

namespace ip2 = ipasir2;

TEST_CASE("solver::set_terminate_callback()")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();

  SECTION("Successfully set and clear terminate callback")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    std::vector<int> num_cb_calls = {0, 0, 0};

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});
    solver->set_terminate_callback([&]() {
      ++num_cb_calls[0];
      return true;
    });
    mock->simulate_terminate_callback_call(1, true);
    CHECK(num_cb_calls == std::vector<int>{1, 0, 0});

    solver->set_terminate_callback([&]() {
      ++num_cb_calls[1];
      return false;
    });
    mock->simulate_terminate_callback_call(1, false);
    CHECK(num_cb_calls == std::vector<int>{1, 1, 0});

    mock->expect_call(1, set_terminate_call{false, IPASIR2_E_OK});
    solver->clear_terminate_callback();

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});
    solver->set_terminate_callback([&]() {
      ++num_cb_calls[2];
      return true;
    });
    mock->simulate_terminate_callback_call(1, true);
    CHECK(num_cb_calls == std::vector<int>{1, 1, 1});
  }


  SECTION("Exception is thrown when set_terminate_callback() fails")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_UNSUPPORTED});
    CHECK_THROWS_AS(solver->set_terminate_callback([&]() { return true; }), ipasir2::ipasir2_error);
  }


  SECTION("When clear_terminate_callback() fails, exception is thrown and old callback is not "
          "called anymore")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    int num_cb_calls = 0;

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});
    solver->set_terminate_callback([&]() {
      ++num_cb_calls;
      return true;
    });

    mock->expect_call(1, set_terminate_call{false, IPASIR2_E_UNKNOWN});
    CHECK_THROWS_AS(solver->clear_terminate_callback(), ipasir2::ipasir2_error);

    mock->simulate_terminate_callback_call(1, false);
    CHECK(num_cb_calls == 0);
  }


  SECTION("When an exception is thrown in the callback, it is rethrown once from solve()")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});
    solver->set_terminate_callback([&]() -> bool { throw std::runtime_error{"test exception"}; });

    // Caveat: this test relies on an implementation detail: callbacks are called during solve(),
    // but here the callback is simulated before the actual solve() call. This keeps some
    // complexity out of the mocking system. If the solver would check for exceptions before
    // invoking ipasir2_solve(), the test would fail though, since that call would not be observed.

    mock->simulate_terminate_callback_call(1, true);

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
