#include <ipasir2cpp.h>

#include "mock/ipasir2_mock_doctest.h"

#include <list>
#include <vector>

#include <doctest.h>

namespace ip2 = ipasir2;

TEST_CASE("solver::add_clause() functions")
{
  auto mock = create_ipasir2_doctest_mock();
  ip2::ipasir2 api = ip2::ipasir2::create();
  std::vector<int32_t> clause_3lits = {1, -2, 3};


  SUBCASE("Successfully add 3-element clause default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add_clause(clause_3lits);
  }


  SUBCASE("Successfully add 3-element clause with non-default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add_clause(clause_3lits, IPASIR2_R_FORGETTABLE);
  }


  SUBCASE("Throws when ipasir2_add() returns error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->add_clause(clause_3lits.begin(), clause_3lits.end()),
                    ip2::ipasir2_error const&);

    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});
    CHECK_THROWS_AS(solver->add_clause(clause_3lits), ip2::ipasir2_error const&);
  }


  SUBCASE("Successfully add empty clause with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{}, IPASIR2_R_NONE, IPASIR2_E_OK});

    std::vector<int32_t> empty_clause;
    auto solver = api.create_solver();
    solver->add_clause(empty_clause.begin(), empty_clause.end());
  }


  SUBCASE("Successfully add 3-element clause from contiguous memory, with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add_clause(clause_3lits.begin(), clause_3lits.end());
  }


  SUBCASE("Successfully add 3-element clause from non-contiguous memory, with default redundancy")
  {
    // std::list is only an example for non-contiguous iterators. In practice, non-contiguous
    // iterators could be used to wrap a function generating clauses one-by-one.
    std::list<int32_t> clause = {1, 2, 3};

    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{clause.begin(), clause.end()}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add_clause(clause.begin(), clause.end());
  }


  SUBCASE("Successfully add 3-element clause with non-default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add_clause(clause_3lits.begin(), clause_3lits.end(), IPASIR2_R_FORGETTABLE);
  }

  CHECK(!mock->has_outstanding_expects());
}
