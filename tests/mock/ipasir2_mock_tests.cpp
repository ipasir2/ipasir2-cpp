#include "ipasir2_mock.h"

#include <vector>

#include <doctest.h>


TEST_CASE("Happy-path example")
{
  auto mock = create_ipasir2_mock();
  std::vector<void*> solvers{2};

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solvers[0]), IPASIR2_E_OK);

  mock->expect_new_instance(2);
  REQUIRE_EQ(ipasir2_init(&solvers[1]), IPASIR2_E_OK);

  std::vector<std::vector<int32_t>> clauses = {{1, 2, -3}, {4, -5}, {-4}};

  mock->expect_call(1, add_call{clauses[0], IPASIR2_R_NONE, IPASIR2_E_OK});
  mock->expect_call(1, solve_call{clauses[2], 10, IPASIR2_E_OK});

  mock->expect_call(2, add_call{clauses[1], IPASIR2_R_EQUIVALENT, IPASIR2_E_UNSUPPORTED});

  CHECK_EQ(ipasir2_add(solvers[0], clauses[0].data(), clauses[0].size(), IPASIR2_R_NONE),
           IPASIR2_E_OK);
  CHECK_EQ(ipasir2_add(solvers[1], clauses[1].data(), clauses[1].size(), IPASIR2_R_EQUIVALENT),
           IPASIR2_E_UNSUPPORTED);

  int result = 0;
  CHECK_EQ(ipasir2_solve(solvers[0], &result, clauses[2].data(), clauses[2].size()), IPASIR2_E_OK);
  CHECK_EQ(result, 10);

  ipasir2_release(solvers[0]);
  ipasir2_release(solvers[1]);
}


TEST_CASE("Test fails if IPASIR2 function is called without a controlling ipasir2_mock_scope"
          * doctest::should_fail() * doctest::no_output())
{
  void* solver = nullptr;
  ipasir2_init(&solver);
}


TEST_CASE("Test fails on unexpected ipasir2_init()" * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  // The next mock ID has not been set in `mock`, so ipasir2_init() fails.
  ipasir2_init(&solver);
}


TEST_CASE("Test fails on unexpected subsequent ipasir2_init()" * doctest::should_fail()
          * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  void* solver1 = nullptr;
  void* solver2 = nullptr;

  mock->expect_new_instance(1);
  ipasir2_init(&solver1);

  // Only one mock ID has not been set in `mock`, so ipasir2_init() fails.
  ipasir2_init(&solver2);
}


TEST_CASE("Test fails when next instance is expected, but ipasir2_init is not called before next "
          "is expected"
          * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  mock->expect_new_instance(1);
  mock->expect_new_instance(2);
}


TEST_CASE("Test fails on unexpected release" * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  int not_a_solver = 42;

  ipasir2_release(&not_a_solver);
}


TEST_CASE("Test fails on double-release of solvers" * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  mock->expect_new_instance(1);
  ipasir2_init(&solver);

  ipasir2_release(solver);
  ipasir2_release(solver);
}


TEST_CASE("Mock allows creation and release of single instance")
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solver), IPASIR2_E_OK);
  REQUIRE_EQ(ipasir2_release(solver), IPASIR2_E_OK);
}


TEST_CASE("Mock allows creation and release of two instances")
{
  auto mock = create_ipasir2_mock();
  std::vector<void*> solvers{2};

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solvers[1]), IPASIR2_E_OK);

  mock->expect_new_instance(2);
  REQUIRE_EQ(ipasir2_init(&solvers[2]), IPASIR2_E_OK);

  REQUIRE_EQ(ipasir2_release(solvers[1]), IPASIR2_E_OK);
  REQUIRE_EQ(ipasir2_release(solvers[2]), IPASIR2_E_OK);
}


TEST_CASE("Mock allows expected ipasir2_add calls")
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solver), IPASIR2_E_OK);

  std::vector<std::vector<int32_t>> clauses = {{1, 2, -3}, {4, -5}};

  SUBCASE("first all expects, then all calls")
  {
    mock->expect_call(1, add_call{clauses[0], IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{clauses[1], IPASIR2_R_EQUIVALENT, IPASIR2_E_UNSUPPORTED});
    CHECK_EQ(ipasir2_add(solver, clauses[0].data(), clauses[0].size(), IPASIR2_R_NONE),
             IPASIR2_E_OK);
    CHECK_EQ(ipasir2_add(solver, clauses[1].data(), clauses[1].size(), IPASIR2_R_EQUIVALENT),
             IPASIR2_E_UNSUPPORTED);
  }

  SUBCASE("interleaved")
  {
    mock->expect_call(1, add_call{clauses[0], IPASIR2_R_NONE, IPASIR2_E_OK});
    CHECK_EQ(ipasir2_add(solver, clauses[0].data(), clauses[0].size(), IPASIR2_R_NONE),
             IPASIR2_E_OK);
    mock->expect_call(1, add_call{clauses[1], IPASIR2_R_EQUIVALENT, IPASIR2_E_UNSUPPORTED});
    CHECK_EQ(ipasir2_add(solver, clauses[1].data(), clauses[1].size(), IPASIR2_R_EQUIVALENT),
             IPASIR2_E_UNSUPPORTED);
  }

  CHECK_EQ(ipasir2_release(solver), IPASIR2_E_OK);
}


TEST_CASE("Test fails when ipasir2_add is expected, but a different function is called instead"
          * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solver), IPASIR2_E_OK);

  std::vector<int32_t> clause = {1, 2, -3};

  mock->expect_call(1, add_call{clause, IPASIR2_R_NONE, IPASIR2_E_OK});

  int result = 0;
  ipasir2_solve(solver, &result, nullptr, 0);
  ipasir2_release(solver);
}


TEST_CASE("Test fails when ipasir2_add is expected, but the solver is released instead"
          * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  void* solver = nullptr;

  mock->expect_new_instance(1);
  REQUIRE_EQ(ipasir2_init(&solver), IPASIR2_E_OK);

  std::vector<int32_t> clause = {1, 2, -3};

  mock->expect_call(1, add_call{clause, IPASIR2_R_NONE, IPASIR2_E_OK});
  ipasir2_release(solver);
}


TEST_CASE("Unreleased instances are detected"
          * doctest::should_fail() * doctest::no_output())
{
  auto mock = create_ipasir2_mock();
  std::vector<void*> solvers{2};

  CHECK(!mock->has_outstanding_expects());
  mock->expect_new_instance(1);
  CHECK(mock->has_outstanding_expects());

  ipasir2_init(&solvers[0]);
  ipasir2_init(&solvers[1]);
  CHECK(mock->has_outstanding_expects());

  ipasir2_release(solvers[0]);
  CHECK(mock->has_outstanding_expects());
  ipasir2_release(solvers[1]);
  CHECK(!mock->has_outstanding_expects());
}
