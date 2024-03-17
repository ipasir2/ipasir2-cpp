#include <ipasir2cpp.h>

#include "mock/ipasir2_mock_doctest.h"

#include <doctest.h>

namespace ip2 = ipasir2;
using ip2::optional_bool;

TEST_CASE("solver::lit_value()")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();


  SUBCASE("Successfully query truth value of a literal")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{-13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{14, -14, IPASIR2_E_OK});
    mock->expect_call(1, val_call{15, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_EQ(solver->lit_value(13), optional_bool{true});
    CHECK_EQ(solver->lit_value(-13), optional_bool{false});
    CHECK_EQ(solver->lit_value(14), optional_bool{false});
    CHECK_EQ(solver->lit_value(15), optional_bool{});
  }


  SUBCASE("Throws when solver indicates error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{2, 2, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_value(2), ip2::ipasir2_error const&);
  }


  SUBCASE("Throws when solver returns invalid value")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{13, 1, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_value(13), ip2::ipasir2_error const&);
  }


  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("solver::lit_failed()")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();


  SUBCASE("Successfully query if a literal is failed")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{2, 1, IPASIR2_E_OK});
    mock->expect_call(1, failed_call{-1, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->lit_failed(2));
    CHECK_FALSE(solver->lit_failed(-1));
  }


  SUBCASE("Throws when solver indicates error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{2, 0, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_failed(2), ip2::ipasir2_error const&);
  }


  SUBCASE("Throws when solver returns invalid value")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{13, -1, IPASIR2_E_OK});
    mock->expect_call(1, failed_call{13, 2, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_failed(13), ip2::ipasir2_error const&);
    CHECK_THROWS_AS(solver->lit_failed(13), ip2::ipasir2_error const&);
  }


  CHECK(!mock->has_outstanding_expects());
}
