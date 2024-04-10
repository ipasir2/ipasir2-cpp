#include <ipasir2cpp.h>

#include "custom_types.h"
#include "ipasir2_mock_factory.h"

#include <catch2/catch_test_macros.hpp>

namespace ip2 = ipasir2;
using ip2::optional_bool;

TEST_CASE("solver::lit_value()")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();


  SECTION("Successfully query truth value of a literal")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{-13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{14, -14, IPASIR2_E_OK});
    mock->expect_call(1, val_call{15, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->lit_value(13) == optional_bool{true});
    CHECK(solver->lit_value(-13) == optional_bool{false});
    CHECK(solver->lit_value(14) == optional_bool{false});
    CHECK(solver->lit_value(15) == optional_bool{});
  }

  SECTION("Successfully query truth value of a literal with custom literal type")
  {
    using custom_lit_test::lit;

    mock->expect_init_call(1);
    mock->expect_call(1, val_call{13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{-13, 13, IPASIR2_E_OK});
    mock->expect_call(1, val_call{14, -14, IPASIR2_E_OK});
    mock->expect_call(1, val_call{15, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->lit_value(lit{13, true}) == optional_bool{true});
    CHECK(solver->lit_value(lit{13, false}) == optional_bool{false});
    CHECK(solver->lit_value(lit{14, true}) == optional_bool{false});
    CHECK(solver->lit_value(lit{15, true}) == optional_bool{});
  }


  SECTION("Throws when solver indicates error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{2, 2, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_value(2), ip2::ipasir2_error);
  }


  SECTION("Throws when solver returns invalid value")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, val_call{13, 1, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->lit_value(13), ip2::ipasir2_error);
  }


  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("solver::assumption_failed()")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();


  SECTION("Successfully query if a literal is failed")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{2, 1, IPASIR2_E_OK});
    mock->expect_call(1, failed_call{-1, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->assumption_failed(2));
    CHECK_FALSE(solver->assumption_failed(-1));
  }


  SECTION("Successfully query if a literal is failed with custom literal type")
  {
    using custom_lit_test::lit;

    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{2, 1, IPASIR2_E_OK});
    mock->expect_call(1, failed_call{-1, 0, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK(solver->assumption_failed(lit{2, true}));
    CHECK_FALSE(solver->assumption_failed(lit{1, false}));
  }


  SECTION("Throws when solver indicates error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{2, 0, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->assumption_failed(2), ip2::ipasir2_error);
  }


  SECTION("Throws when solver returns invalid value")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, failed_call{13, -1, IPASIR2_E_OK});
    mock->expect_call(1, failed_call{13, 2, IPASIR2_E_OK});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->assumption_failed(13), ip2::ipasir2_error);
    CHECK_THROWS_AS(solver->assumption_failed(13), ip2::ipasir2_error);
  }


  CHECK(!mock->has_outstanding_expects());
}
