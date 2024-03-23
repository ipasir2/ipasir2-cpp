#include <ipasir2cpp.h>

#include "example_utils.h"

#include <iostream>
#include <vector>


namespace ip2 = ipasir2;

constexpr std::string_view cnf_file = "tiny-planning-sat.cnf";


void example_03_find_backbones()
{
  PRINT_FILENAME();

  ip2::ipasir2 api = ip2::ipasir2::create();
  auto solver = api.create_solver();

  dimacs_parser parser{path_of_cnf(cnf_file)};
  parser.for_each_clause([&](std::span<int32_t const> clause) { solver->add(clause); });

  std::vector<int32_t> backbone_vars;
  int32_t num_undecided_vars = 0;

  for (int32_t var = 1; var <= parser.max_var(); ++var) {
    try {
      if (!solver->solve(var).unwrap()) {
        backbone_vars.push_back(-var);
      }
      else if (!solver->solve(-var).unwrap()) {
        backbone_vars.push_back(var);
      }
    }
    catch (std::bad_optional_access const&) {
      ++num_undecided_vars;
    }
  }

  print("  {} of {} variables are backbones, {} checks failed. Backbones: {}",
        backbone_vars.size(),
        parser.max_var(),
        num_undecided_vars,
        to_string(backbone_vars));
}
