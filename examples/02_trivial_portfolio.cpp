#include <ipasir2cpp.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "example_utils.h"

namespace ip2 = ipasir2;
using namespace std::chrono_literals;


constexpr size_t num_threads = 2;
constexpr std::chrono::duration timeout = 5s;
constexpr std::string_view cnf_file = "example.cnf";


void example_02_trivial_portfolio()
{
  PRINT_FILENAME();

  ip2::ipasir2 api = ip2::ipasir2::create();

  std::vector<std::unique_ptr<ip2::solver>> solvers;
  for (size_t idx = 0; idx < num_threads; ++idx) {
    solvers.push_back(api.create_solver());
  }

  dimacs_parser parser{path_of_cnf(cnf_file)};
  parser.for_each_clause([&](std::span<int32_t const> clause) {
    for (auto const& solver : solvers) {
      solver->add(clause);
    }
  });

  // TODO: diversify solver instances via options interface (ipasir.phase.initial,
  // ipasir.vsids.initial)

  std::atomic<ip2::optional_bool> result;

  // scope for joining threads
  {
    std::vector<std::jthread> threads;
    for (size_t idx = 0; idx < solvers.size(); ++idx) {
      threads.emplace_back([&result, solver = std::move(solvers[idx])]() {
        solver->set_terminate_callback([&, s = stopwatch{}] {
          return s.time_since_start() >= timeout || result.load().has_value();
        });

        if (auto local_result = solver->solve(); local_result.has_value()) {
          result.store(local_result);
        }
      });
    }
  }

  print("  Result: {}", ip2::to_string(result.load()));
}
