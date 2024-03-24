#include <ipasir2cpp.h>

#include "example_utils.h"

namespace ip2 = ipasir2;


// This clause type has begin() and end() methods for iterating over the
// literals. Objects of this type can be directly passed to solver::add().
class own_clause {
public:
  own_clause(std::vector<int32_t> const& lits) : m_literals{lits} {}

  auto begin() const { return m_literals.begin(); }
  auto end() const { return m_literals.end(); }

private:
  std::vector<int32_t> m_literals;
  int m_additional_attrs = 0;
};


namespace third_party_lib {
// This clause type does not have begin() and end(). Suppose you don't
// own this type. It can still be passed to solver::add() if you define
// suitable begin() and end() functions in the same namespace as the
// third-party clause type - they can return iterators, or pointers.
class clause {
public:
  clause(std::vector<int32_t> const& lits) : m_literals{lits} {}

  std::vector<int32_t> const& literals() const { return m_literals; }

private:
  std::vector<int32_t> m_literals;
  int m_additional_attrs = 0;
};


auto begin(clause const& clause)
{
  // begin() and end() need to return iterators, or pointers to a
  // literal buffer.
  return clause.literals().begin();
}


auto end(clause const& clause)
{
  return clause.literals().end();
}
}


void example_04_custom_types()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::ipasir2::create();
    std::unique_ptr<ip2::solver> solver = api.create_solver();

    own_clause clause1{{1, 2, 3}};
    third_party_lib::clause clause2{{1, 2}};

    // solver->add() can be used with multiple clause types. The
    // clause type is not part of the ip2::solver type.
    solver->add(clause1);
    solver->add(clause2);

    // etc.
  }
  catch (ip2::ipasir2_error const& error) {
    std::cerr << std::format("Could not solve the formula: {}\n", error.what());
  }
}
