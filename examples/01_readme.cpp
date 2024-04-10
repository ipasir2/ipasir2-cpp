/// A trivial example showing how to solve the formula `{{1, 2, 3}, {-1}, {-2}}`
/// with ipasir2-cpp.

#include <ipasir2cpp.h>

#include <string_view>

#include "example_utils.h"

namespace ip2 = ipasir2;
using namespace std::literals;


void example_01_readme()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::create_api();
    // For dynamic loading: ip2::ipasir2 api = ip2::create_api("./solver.so");

    std::unique_ptr<ip2::solver> solver = api.create_solver();
    solver->add(1, 2, 3);
    solver->add(-1);
    solver->add(-2);

    auto result = solver->solve();
    print("Result: {}", result.map("sat"sv, "unsat"sv, "unknown"sv));
  }
  catch (ip2::ipasir2_error const& error) {
    print("Could not solve the formula: {}", error.what());
  }
}
