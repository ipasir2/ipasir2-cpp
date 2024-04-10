#pragma once

#include "mock/ipasir2_mock.h"

// Required by doctest.h when using Microsoft STL
#include <iostream>

#include <doctest.h>


inline std::unique_ptr<ipasir2_mock, decltype(&delete_ipasir2_mock)> create_ipasir2_doctest_mock()
{
  auto result = std::unique_ptr<ipasir2_mock, decltype(&delete_ipasir2_mock)>(new_ipasir2_mock(),
                                                                              delete_ipasir2_mock);
  result->set_fail_observer([](std::string_view message) { FAIL(message); });
  return result;
}
