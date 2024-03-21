#include <ipasir2cpp.h>

#include "ipasir2_mock_doctest.h"

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("solver::set_terminate_callback()")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();

  SUBCASE("Successfully set and clear terminate callback")
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
    CHECK_EQ(num_cb_calls, std::vector<int>{1, 0, 0});

    solver->set_terminate_callback([&]() {
      ++num_cb_calls[1];
      return false;
    });
    mock->simulate_terminate_callback_call(1, false);
    CHECK_EQ(num_cb_calls, std::vector<int>{1, 1, 0});

    mock->expect_call(1, set_terminate_call{false, IPASIR2_E_OK});
    solver->clear_terminate_callback();

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});
    solver->set_terminate_callback([&]() {
      ++num_cb_calls[2];
      return true;
    });
    mock->simulate_terminate_callback_call(1, true);
    CHECK_EQ(num_cb_calls, std::vector<int>{1, 1, 1});
  }


  SUBCASE("Exception is thrown when set_terminate_callback fails")
  {
    mock->expect_init_call(1);
    auto solver = api.create_solver();

    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_UNSUPPORTED});
    CHECK_THROWS_AS(solver->set_terminate_callback([&]() { return true; }),
                    ipasir2::ipasir2_error const&);
  }


  SUBCASE("When clear_terminate_callback() fails, exception is thrown and old callback is not "
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
    CHECK_THROWS_AS(solver->clear_terminate_callback(), ipasir2::ipasir2_error const&);

    mock->simulate_terminate_callback_call(1, false);
    CHECK_EQ(num_cb_calls, 0);
  }

  CHECK(!mock->has_outstanding_expects());
}
