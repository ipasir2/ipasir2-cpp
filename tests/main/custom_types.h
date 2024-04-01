#pragma once

#include <cstdint>
#include <vector>


class custom_lit_container_1 {
public:
  custom_lit_container_1(std::vector<int32_t> const& lits) : m_literals{lits} {}

  auto begin() const noexcept { return m_literals.begin(); }
  auto end() const noexcept { return m_literals.end(); }

private:
  std::vector<int32_t> m_literals;
};


namespace adl_test {
class custom_lit_container_2 {
public:
  custom_lit_container_2(std::vector<int32_t> const& lits) : m_literals{lits} {}
  std::vector<int32_t> const& literals() const { return m_literals; }

private:
  std::vector<int32_t> m_literals;
};


inline auto begin(custom_lit_container_2 const& clause)
{
  return clause.literals().begin();
}


inline auto end(custom_lit_container_2 const& clause)
{
  return clause.literals().end();
}
}


namespace custom_lit_test {
class lit {
public:
  lit(int var, bool sign) : m_value{2 * var + static_cast<int>(sign)} {}

  bool sign() const { return m_value & 1; }

  int var() const { return m_value >> 1; }

private:
  int m_value = 0;
};


inline int32_t to_ipasir2_lit(lit const& literal)
{
  return literal.var() * (literal.sign() ? 1 : -1);
}


inline lit from_ipasir2_lit(int32_t literal)
{
  return lit(std::abs(literal), literal > 0);
}
}
