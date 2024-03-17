#include <ipasir2cpp.h>

#include "mock/ipasir2_mock.h"

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("Query signature")
{
  auto mock = create_ipasir2_mock();
  ip2::ipasir2 api;


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
}
