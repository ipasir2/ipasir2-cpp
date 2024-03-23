#include <ipasir2cpp.h>

#include "example_utils.h"

namespace ip2 = ipasir2;


void example_01_readme()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::ipasir2::create();
    // For dynamic loading: ip2::ipasir2 api = ip2::ipasir2::create("./solver.so");

    std::unique_ptr<ip2::solver> solver = api.create_solver();
    solver->add(1, 2, 3);
    solver->add(-1);
    solver->add(-2);

    if (auto result = solver->solve(); result.has_value()) {
      std::cout << std::format("Result: {}\n", result.unwrap() ? "SAT" : "UNSAT");
    }
    else {
      std::cout << "The solver did not produce a result.\n";
    }
  }
  catch (ip2::ipasir2_error const& error) {
    std::cerr << std::format("Could not solve the formula: {}\n", error.what());
  }
}
