#include <ipasir2cpp.h>

#include "ipasir2_mock_factory.h"

#include <catch2/catch_test_macros.hpp>

namespace ip2 = ipasir2;

TEST_CASE("Query signature")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();


  SECTION("Successfully query signature")
  {
    mock->set_signature("test 1.0", IPASIR2_E_OK);
    CHECK(api.signature() == "test 1.0");
  }


  SECTION("Throws when querying the signature is unsupported")
  {
    mock->set_signature("", IPASIR2_E_UNSUPPORTED);
    CHECK_THROWS_AS(api.signature(), ip2::ipasir2_error);
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("Instantiate solver")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();


  SECTION("Successfully instantiate solver")
  {
    mock->expect_init_call(1);
    api.create_solver();
  }


  SECTION("Throws when creating instance fails")
  {
    mock->expect_init_call_and_fail(IPASIR2_E_UNKNOWN);
    CHECK_THROWS_AS(api.create_solver(), ip2::ipasir2_error);
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("Get IPASIR-2 handle")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();

  {
    mock->expect_init_call(1);
    auto solver1 = api.create_solver();
    mock->expect_init_call(2);
    auto solver2 = api.create_solver();

    CHECK(solver1->get_ipasir2_handle() == mock->get_ipasir2_handle(1));
    CHECK(solver2->get_ipasir2_handle() == mock->get_ipasir2_handle(2));
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("optional_bool")
{
  CHECK(ip2::optional_bool{true}.map(1, 2, 3) == 1);
  CHECK(ip2::optional_bool{false}.map(1, 2, 3) == 2);
  CHECK(ip2::optional_bool{}.map(1, 2, 3) == 3);
}
