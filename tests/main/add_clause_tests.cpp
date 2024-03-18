#include <ipasir2cpp.h>

#include "ipasir2_mock_doctest.h"

#include <list>
#include <vector>

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("solver::add() functions")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();
  std::vector<int32_t> clause_3lits = {1, -2, 3};

  using ip2::redundancy;


  SUBCASE("Successfully add clauses via parameter-pack function")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{1}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3, -4}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(1);
    solver->add(1, -2);
    solver->add(1, -2, 3);
    solver->add(1, -2, 3, -4);
  }


  SUBCASE("Successfully add clauses via parameter-pack function with non-default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{1}, IPASIR2_R_EQUIVALENT, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_EQUISATISFIABLE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3}, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3, -4}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(redundancy::equivalent, 1);
    solver->add(redundancy::equisatisfiable, 1, -2);
    solver->add(redundancy::forgettable, 1, -2, 3);
    solver->add(redundancy::none, 1, -2, 3, -4);
  }


  SUBCASE("Successfully add 3-element clause default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits);
  }


  SUBCASE("Successfully add 3-element clause with non-default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(redundancy::forgettable, clause_3lits);
  }


  SUBCASE("Throws when ipasir2_add() returns error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->add(clause_3lits.begin(), clause_3lits.end()),
                    ip2::ipasir2_error const&);

    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});
    CHECK_THROWS_AS(solver->add(clause_3lits), ip2::ipasir2_error const&);

    mock->expect_call(1, add_call{{1}, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});
    CHECK_THROWS_AS(solver->add(1), ip2::ipasir2_error const&);
  }


  SUBCASE("Successfully add empty clause with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{}, IPASIR2_R_NONE, IPASIR2_E_OK});

    std::vector<int32_t> empty_clause;
    auto solver = api.create_solver();
    solver->add(empty_clause.begin(), empty_clause.end());
  }


  SUBCASE("Successfully add 3-element clause from contiguous memory, with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits.begin(), clause_3lits.end());
  }


  SUBCASE("Successfully add 3-element clause from non-contiguous memory, with default redundancy")
  {
    // std::list is only an example for non-contiguous iterators. In practice, non-contiguous
    // iterators could be used to wrap a function generating clauses one-by-one.
    std::list<int32_t> clause = {1, 2, 3};

    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{clause.begin(), clause.end()}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause.begin(), clause.end());
  }


  SUBCASE("Successfully add 3-element clause with non-default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(redundancy::forgettable, clause_3lits.begin(), clause_3lits.end());
  }


  CHECK(!mock->has_outstanding_expects());
}
