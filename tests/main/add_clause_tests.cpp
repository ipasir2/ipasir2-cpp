#include <ipasir2cpp.h>

#include "custom_types.h"
#include "ipasir2_mock_factory.h"

#include <list>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace ip2 = ipasir2;

TEST_CASE("solver::add() functions")
{
  auto mock = create_ipasir2_test_mock();
  ip2::ipasir2 api = ip2::create_api();
  std::vector<int32_t> clause_3lits = {1, -2, 3};

  using ip2::redundancy;


  SECTION("Successfully add clauses via parameter-pack function")
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


  SECTION("Successfully add clauses via parameter-pack function with non-default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{1}, IPASIR2_R_EQUIVALENT, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_EQUISATISFIABLE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3}, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3, -4}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(1, redundancy::equivalent);
    solver->add(1, -2, redundancy::equisatisfiable);
    solver->add(1, -2, 3, redundancy::forgettable);
    solver->add(1, -2, 3, -4, redundancy::none);
  }


  SECTION("Successfully add clauses via parameter-pack function with custom literal type")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{1}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3, -4}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2, 3}, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    using custom_lit_test::lit;

    auto solver = api.create_solver();
    solver->add(lit{1, true});
    solver->add(lit{1, true}, lit{2, false});
    solver->add(lit{1, true}, lit{2, false}, lit{3, true});
    solver->add(lit{1, true}, lit{2, false}, lit{3, true}, lit{4, false});
    solver->add(lit{1, true}, lit{2, false}, lit{3, true}, redundancy::forgettable);
  }


  SECTION("Successfully add 3-element clause default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits);
  }


  SECTION("Successfully add 3-element clause with non-default redundancy with container-based add")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits, redundancy::forgettable);
  }


  SECTION("Throws when ipasir2_add() returns error")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});

    auto solver = api.create_solver();
    CHECK_THROWS_AS(solver->add(clause_3lits.begin(), clause_3lits.end()), ip2::ipasir2_error);

    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});
    CHECK_THROWS_AS(solver->add(clause_3lits), ip2::ipasir2_error);

    mock->expect_call(1, add_call{{1}, IPASIR2_R_NONE, IPASIR2_E_INVALID_ARGUMENT});
    CHECK_THROWS_AS(solver->add(1), ip2::ipasir2_error);
  }


  SECTION("Successfully add empty clause with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{}, IPASIR2_R_NONE, IPASIR2_E_OK});

    std::vector<int32_t> empty_clause;
    auto solver = api.create_solver();
    solver->add(empty_clause.begin(), empty_clause.end());
  }


  SECTION("Successfully add 3-element clause from contiguous memory, with default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits.begin(), clause_3lits.end());
  }


  SECTION("Successfully add 3-element clause from non-contiguous memory, with default redundancy")
  {
    // std::list is only an example for non-contiguous iterators. In practice, non-contiguous
    // iterators could be used to wrap a function generating clauses one-by-one.
    std::list<int32_t> clause = {1, 2, 3};

    mock->expect_init_call(1);
    mock->expect_call(1, add_call{{clause.begin(), clause.end()}, IPASIR2_R_NONE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause.begin(), clause.end());
  }


  SECTION("Successfully add 3-element clause with non-default redundancy")
  {
    mock->expect_init_call(1);
    mock->expect_call(1, add_call{clause_3lits, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    auto solver = api.create_solver();
    solver->add(clause_3lits.begin(), clause_3lits.end(), redundancy::forgettable);
  }


  SECTION("Successfully add clauses with custom clause type")
  {
    mock->expect_init_call(1);

    mock->expect_call(1, add_call{{1, 2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 3}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 4}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 5}, IPASIR2_R_NONE, IPASIR2_E_OK});

    custom_lit_container_1 clause1{{1, 2}};
    custom_lit_container_1 const clause2{{1, 3}};
    adl_test::custom_lit_container_2 clause3{{1, 4}};
    adl_test::custom_lit_container_2 const clause4{{1, 5}};

    auto solver = api.create_solver();
    solver->add(clause1);
    solver->add(clause2);
    solver->add(clause3);
    solver->add(clause4);
  }


  SECTION("Successfully add clauses with custom literal type")
  {
    mock->expect_init_call(1);

    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 3}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -4}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 5}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, 3}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{1, -2}, IPASIR2_R_FORGETTABLE, IPASIR2_E_OK});

    using custom_lit = custom_lit_test::lit;

    // With contiguous iterator:
    std::vector<custom_lit> clause1{{1, true}, {2, false}};
    std::vector<custom_lit> const clause2{{1, true}, {3, true}};

    // With non-contiguous iterator:
    std::list<custom_lit> clause3{{1, true}, {4, false}};
    std::list<custom_lit> const clause4{{1, true}, {5, true}};

    auto solver = api.create_solver();
    solver->add(clause1);
    solver->add(clause2);
    solver->add(clause3);
    solver->add(clause4);
    solver->add(clause1.data(), clause1.data() + clause1.size());
    solver->add(clause2.data(), clause2.data() + clause2.size());
    solver->add(clause1, redundancy::forgettable);
  }

  CHECK(!mock->has_outstanding_expects());
}
