#include <ipasir2cpp.h>

#include "ipasir2_mock_factory.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

#if __has_include(<version>)
#include <version>
#endif


namespace ip2 = ipasir2;

using clause_vec = std::vector<std::vector<int32_t>>;

TEST_CASE("solver::get_option() and solver::set_option()")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();

  std::vector<ipasir2_option> test_options
      = {ipasir2_option{"test_option_1", -1000, 1000, IPASIR2_S_CONFIG, 1, 0, nullptr},
         ipasir2_option{"test_option_2", 0, 100, IPASIR2_S_SOLVING, 0, 1, nullptr},
         ipasir2_option{nullptr, 0, 0, IPASIR2_S_SOLVING, 0, 0, nullptr}};


  SECTION("Successfully call get_option()")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);

    mock->expect_call(1, options_call{IPASIR2_E_OK}); // cached, hence only called once

    auto solver = api.create_solver();

    ip2::option opt1 = solver->get_option("test_option_1");
    CHECK(opt1.name() == "test_option_1");
    CHECK(opt1.min_value() == -1000);
    CHECK(opt1.max_value() == 1000);
    CHECK(opt1.is_tunable());
    CHECK(!opt1.is_indexed());


    ip2::option opt2 = solver->get_option("test_option_2");
    CHECK(opt2.name() == "test_option_2");
    CHECK(opt2.min_value() == 0);
    CHECK(opt2.max_value() == 100);
    CHECK(!opt2.is_tunable());
    CHECK(opt2.is_indexed());
  }


  SECTION("get_option() throws when option is unknown")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->get_option("unknown test option"), ip2::ipasir2_error);
  }


  SECTION("get_option() throws when ipasir2_options() indicates an error")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_UNSUPPORTED});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->get_option("test_option_2"), ip2::ipasir2_error);
  }


  SECTION("Successfully call set_option() with option handle")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_OK});
    mock->expect_call(1, set_option_call{"test_option_2", 2, 5, IPASIR2_E_OK});

    auto solver = api.create_solver();
    ip2::option opt = solver->get_option("test_option_2");
    solver->set_option(opt, 2, 5);
  }


  SECTION("Successfully call set_option() with option name")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_OK});
    mock->expect_call(1, set_option_call{"test_option_2", 2, 5, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->set_option("test_option_2", 2, 5);
  }


  SECTION("set_option() throws when ipasir2_set_option() indicates an error")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_OK});
    mock->expect_call(1, set_option_call{"test_option_2", 2, 500, IPASIR2_E_UNSUPPORTED_ARGUMENT});

    auto solver = api.create_solver();
    ip2::option opt = solver->get_option("test_option_2");
    CHECK_THROWS_AS(solver->set_option(opt, 2, 500), ip2::ipasir2_error);
  }


  SECTION("has_options() checks whether options are present")
  {
    mock->expect_init_call(1);
    mock->set_options(1, test_options);
    mock->expect_call(1, options_call{IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->has_option("test_option_1"));
    CHECK(solver->has_option("test_option_2"));
    CHECK(!solver->has_option("unknown_option"));
  }


  CHECK(!mock->has_outstanding_expects());
}
