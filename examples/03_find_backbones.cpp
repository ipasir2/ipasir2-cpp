#include <ipasir2cpp.h>

#include "example_utils.h"

#include <chrono>
#include <vector>


namespace ip2 = ipasir2;
using namespace std::chrono_literals;

constexpr std::string_view cnf_file = "tiny-planning-sat.cnf";


std::vector<int32_t> get_assignment(ip2::solver const& solver, int32_t max_var)
{
  std::vector<int32_t> result;
  for (int32_t var = 1; var <= max_var; ++var) {
    ip2::optional_bool const var_value = solver.lit_value(var);
    if (var_value.has_value()) {
      result.push_back(var_value.map(var, -var, 0));
    }
  }
  return result;
}


void example_03_find_backbones()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::ipasir2::create();
    auto solver = api.create_solver();

    dimacs_parser parser{path_of_cnf(cnf_file)};
    parser.for_each_clause([&](std::span<int32_t const> clause) { solver->add(clause); });

    solver->set_terminate_callback([&, s = stopwatch{}] { return s.time_since_start() >= 20s; });

    if (ip2::optional_bool initial_result = solver->solve(); initial_result != ip2::optional_bool{true}) {
      print("{}", !initial_result.has_value() ? "   Timeout" : "  The formula is not satisfiable, aborting");
      return;
    }

    // backbone_candidates contains backbone literal candidates. When a candidate literal is
    // determined not to be a backbone, it is replaced by 0.
    std::vector<int32_t> backbone_candidates = get_assignment(*solver, parser.max_var());
    std::vector<int32_t> backbones;

    for (auto candidate = backbone_candidates.begin(); candidate != backbone_candidates.end();
         ++candidate) {
      if (*candidate == 0) {
        continue;
      }

      ip2::optional_bool const counterexample_search_result = solver->solve(-*candidate);
      if (!counterexample_search_result.has_value()) {
        print("  Timeout after finding {} backbone literals", *candidate, backbones.size());
        return;
      }

      if (counterexample_search_result == ip2::optional_bool{true}) {
        // The found model might eliminate further backbone candidates that have not been
        // checked yet:
        for (auto rest_iter = candidate; rest_iter != backbone_candidates.end(); ++rest_iter) {
          if (*rest_iter != 0) {
            *rest_iter = solver->lit_value(*rest_iter).map(*rest_iter, 0, 0);
          }
        }
      }
      else {
        solver->add(*candidate);
        backbones.push_back(*candidate);
      }
    }

    print("  {} of {} variables are backbones: {}",
          backbones.size(),
          parser.max_var(),
          to_string(backbones));
  }
  catch (parse_error const& error) {
    print("  Failed parsing {}: {}", cnf_file, error.what());
    return;
  }
  catch (ip2::ipasir2_error const& error) {
    print("  Failed solving {}: {}", cnf_file, error.what());
  }
}
