/// This example shows how to use custom clause and literal types with the IPASIR-2 wrapper.

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
  // ... additional members ...
};


namespace third_party_lib {
// The following clause type does not have begin() and end(). Suppose it is defined
// in a third-party library and can't be changed. It can still be passed to
// solver::add() if you define suitable begin() and end() functions in the same
// namespace as the third-party clause type .
class clause {
public:
  clause(std::vector<int32_t> const& lits) : m_literals{lits} {}

  std::vector<int32_t> const& literals() const { return m_literals; }

private:
  std::vector<int32_t> m_literals;
  // ... additional members ...
};


// The freestanding begin() and end() functions are used via ADL.
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


namespace custom_lit {
// The following literal type is similar to the one used in Minisat.
// To use these literals with the IPASIR-2 wrapper, you need to define
// conversion functions from and to DIMACS-style representation. This
// is done by specializing `ipasir2::lit_traits` for your literal type,
// like in the following:
class lit {
public:
  lit(uint32_t var, bool sign) : m_value{2 * var + static_cast<int32_t>(sign)} {}

  bool sign() const { return m_value & 1; }
  int var() const { return m_value >> 1; }
  size_t index() const { return m_value; }

  // etc.

private:
  uint32_t m_value = 0;
};
}


namespace ipasir2 {
template <>
struct lit_traits<custom_lit::lit> {
  static int32_t to_ipasir2_lit(custom_lit::lit const& literal)
  {
    return literal.var() * (literal.sign() ? 1 : -1);
  }


  static custom_lit::lit from_ipasir2_lit(int32_t literal)
  {
    return custom_lit::lit(std::abs(literal), literal > 0);
  }
};
}


void example_04_custom_types()
{
  PRINT_FILENAME();

  try {
    ip2::ipasir2 api = ip2::create_api();
    std::unique_ptr<ip2::solver> solver = api.create_solver();

    own_clause clause1{{1, 2, 3}};
    third_party_lib::clause clause2{{1, 2}};

    // solver->add() can be used with multiple clause types. The
    // clause type is not part of the ip2::solver type.
    solver->add(clause1);
    solver->add(clause2);


    // Custom literal types can also be used with the ipasir2 wrapper:
    using custom_lit::lit;
    std::vector<lit> clause3{lit{1, true}, lit{2, false}};
    solver->add(clause3);

    // etc.
  }
  catch (ip2::ipasir2_error const& error) {
    std::cerr << std::format("Could not solve the formula: {}\n", error.what());
  }
}
