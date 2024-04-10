/// This example shows how to use the option interface in IPASIR-2, and how to
/// run multiple IPASIR-2 solvers concurrently.

#include <ipasir2cpp.h>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <unordered_map>

#include "example_utils.h"

namespace ip2 = ipasir2;
using namespace std::chrono_literals;


constexpr size_t num_threads = 2;
constexpr std::chrono::duration timeout = 5s;
constexpr std::string_view cnf_file = "example.cnf";


void diversify(ip2::solver& solver, size_t solver_index, int32_t max_var);


void example_02_trivial_portfolio()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::create_api();

    std::vector<std::unique_ptr<ip2::solver>> solvers;
    for (size_t idx = 0; idx < num_threads; ++idx) {
      solvers.push_back(api.create_solver());
    }

    int32_t max_var = 0;

    try {
      dimacs_parser parser{path_of_cnf(cnf_file)};
      parser.for_each_clause([&](std::span<int32_t const> clause) {
        for (auto const& solver : solvers) {
          solver->add(clause);
        }
      });
      max_var = parser.max_var();
    }
    catch (parse_error const& error) {
      print("Failed parsing {}: {}", cnf_file, error.what());
      return;
    }


    std::atomic<ip2::optional_bool> result;

    // scope for joining threads
    {
      std::vector<std::jthread> threads;
      for (size_t idx = 0; idx < solvers.size(); ++idx) {
        threads.emplace_back([&result, max_var, idx, solver = std::move(solvers[idx])]() {
          try {
            solver->set_terminate_callback([&, s = stopwatch{}] {
              return s.time_since_start() >= timeout || result.load().has_value();
            });

            solver->set_option("ipasir.yolo", 1); // enables one-shot solving
            diversify(*solver, idx, max_var);

            if (auto local_result = solver->solve(); local_result.has_value()) {
              result.store(local_result);
            }
          }
          catch(ip2::ipasir2_error const& error) {
            print("  Solver failed in thread {}: {}", idx, error.what());
          }
        });
      }
    }

    print("  Result: {}", ip2::to_string(result.load()));
  }
  catch (ip2::ipasir2_error const& error) {
    print("Failed solving {}: {}", cnf_file, error.what());
  }
}


void diversify(ip2::solver& solver, size_t solver_index, int32_t max_var)
{
  // Since option lookup via name is O(|options|) and the options are
  // set for each variable, the option handles are looked up once here:
  ip2::option vsids_opt = solver.get_option("ipasir.vsids.initial");
  ip2::option phase_opt = solver.get_option("ipasir.phase.initial");

  // Naive pseudorandom number generation, for illustration only:
  std::mt19937 rng{solver_index};
  std::uniform_int_distribution<int64_t> vsids_scores(vsids_opt.min_value(), vsids_opt.max_value());
  for (int32_t var = 1; var < max_var; ++var) {
    solver.set_option(vsids_opt, vsids_scores(rng), var);
    solver.set_option(phase_opt, rng() % 2, var);
  }
}
