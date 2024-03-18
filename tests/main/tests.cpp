#include <ipasir2cpp.h>

#include "ipasir2_mock_doctest.h"

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("Query signature")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();


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
  ip2::ipasir2 api = ip2::ipasir2::create();


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
