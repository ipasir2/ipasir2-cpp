#pragma once

#include "ipasir2_mock.h"

#include <doctest.h>


inline std::unique_ptr<ipasir2_mock> create_ipasir2_doctest_mock()
{
  auto result = std::unique_ptr<ipasir2_mock>(new_ipasir2_mock());
  result->set_fail_observer([](std::string_view message) { FAIL(message); });
  return result;
}
