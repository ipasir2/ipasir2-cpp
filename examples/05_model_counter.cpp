#include <ipasir2cpp.h>

#include "example_utils.h"

#include <vector>
#include <span>
#include <string_view>


namespace ip2 = ipasir2;

constexpr std::string_view cnf_file = "tiny-planning-sat.cnf";


struct models {
  std::vector<int32_t> exclusion_clause;
  uint64_t size = 1;
};

models evaluate_satisfied(ip2::solver const& solver, int32_t max_var)
{
  models result;

  for (int32_t var = 1; var < max_var; ++var) {
    ip2::optional_bool val = solver.lit_value(var);

    if (val.has_value()) {
      result.exclusion_clause.push_back(val.unwrap() ? -var : var);
    }
    else {
      result.size *= 2;
    }
  }

  return result;
}


void example_05_model_counter()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::ipasir2::create();
    auto solver = api.create_solver();

    dimacs_parser parser{path_of_cnf(cnf_file)};
    parser.for_each_clause([&](std::span<int32_t const> clause) { solver->add(clause); });

    uint64_t num_models = 0;

    ip2::optional_bool result = solver->solve();
    while (result == ip2::optional_bool{true}) {
      models models = evaluate_satisfied(*solver, parser.max_var());
      solver->add(models.exclusion_clause);
      num_models += models.size;
      result = solver->solve();
    }

    if (!result.has_value()) {
      print("  Aborted after finding {} models.", num_models);
    }
    else {
      print("  Number of models found: {}", num_models);
    }
  }
  catch (parse_error const& error) {
    print("Failed parsing {}: {}", cnf_file, error.what());
    return;
  }
  catch (ip2::ipasir2_error const& error) {
    print("Failed solving {}: {}", cnf_file, error.what());
  }
}
