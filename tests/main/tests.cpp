#include <ipasir2cpp.h>

#include "ipasir2_mock_doctest.h"

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("Query signature")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::create_api();


  SUBCASE("Successfully query signature")
  {
    mock->set_signature("test 1.0", IPASIR2_E_OK);
    CHECK_EQ(api.signature(), "test 1.0");
  }


  SUBCASE("Throws when querying the signature is unsupported")
  {
    mock->set_signature("", IPASIR2_E_UNSUPPORTED);
    CHECK_THROWS_AS(api.signature(), ip2::ipasir2_error const&);
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("Instantiate solver")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::create_api();


  SUBCASE("Successfully instantiate solver")
  {
    mock->expect_init_call(1);
    api.create_solver();
  }


  SUBCASE("Throws when creating instance fails")
  {
    mock->expect_init_call_and_fail(IPASIR2_E_UNKNOWN);
    CHECK_THROWS_AS(api.create_solver(), ip2::ipasir2_error const&);
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("Get IPASIR-2 handle")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::create_api();

  {
    mock->expect_init_call(1);
    auto solver1 = api.create_solver();
    mock->expect_init_call(2);
    auto solver2 = api.create_solver();

    CHECK_EQ(solver1->get_ipasir2_handle(), mock->get_ipasir2_handle(1));
    CHECK_EQ(solver2->get_ipasir2_handle(), mock->get_ipasir2_handle(2));
  }

  CHECK(!mock->has_outstanding_expects());
}


TEST_CASE("optional_bool")
{
  CHECK_EQ(ip2::optional_bool{true}.map(1, 2, 3), 1);
  CHECK_EQ(ip2::optional_bool{false}.map(1, 2, 3), 2);
  CHECK_EQ(ip2::optional_bool{}.map(1, 2, 3), 3);
}
