#pragma once

#if __cplusplus < 201703L
#error "ipasir2cpp.h requires C++17 or newer"
#endif

#include <ipasir2.h>

#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include(<version>)
#include <version>
#endif


namespace ipasir2 {

class ipasir2_error : public std::runtime_error {
public:
  explicit ipasir2_error(ipasir2_errorcode) : runtime_error("IPASIR2 call failed"){};
  explicit ipasir2_error(std::string_view message) : runtime_error(message.data()){};
};


/// Restricted version of std::optional<bool>
///
/// In contrast to std::bool, this class does not have implicit
/// conversion to bool, no comparison operators for bool, and no
/// unchecked access to the value.
class optional_bool {
public:
  explicit optional_bool(bool value) : m_value{value} {}

  /// Constructs an empty optional_bool
  optional_bool() {}


  /// Returns the contained value.
  ///
  /// \throws std::bad_optional_access if the object has no value.
  bool unwrap() const { return m_value.value(); }

  bool has_value() const { return m_value.has_value(); }

  std::optional<bool> as_std_optional() const { return m_value; }

  auto operator==(optional_bool const& rhs) const noexcept
  {
    return this == (&rhs) || m_value == rhs.m_value;
  }

  auto operator!=(optional_bool const& rhs) const noexcept { return !(*this == rhs); }

  // Rationale: This is used for instance to make solver::solve() calls less
  // errorprone. If that function would return an
  // std::optional<bool>, conditions like `if(solver.solve()) { foo(); }`
  // would become hard to read, because then, `foo();` would be
  // executed. Likewise, if the result would be an `enum class result { sat, unsat,
  // unknown }`, clients could get subtle bugs by only comparing the
  // result to `result::sat` and ignoring the case `result::unknown`.

private:
  std::optional<bool> m_value;
};


namespace detail {
  inline void throw_if_failed(ipasir2_errorcode errorcode)
  {
    if (errorcode != IPASIR2_E_OK) {
      throw ipasir2_error{errorcode};
    }
  }


#if __cpp_lib_concepts
  template <typename T>
  constexpr bool is_contiguous_lit_iter = std::contiguous_iterator<T>;
#else
  template <typename T>
  constexpr bool is_contiguous_lit_iter
      = std::is_pointer_v<std::decay_t<T>>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::iterator>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::const_iterator>;
#endif


  template <typename Iter>
  std::pair<int32_t const*, size_t>
  as_contiguous(Iter start, Iter stop, std::vector<int32_t>& buffer)
  {
    if constexpr (detail::is_contiguous_lit_iter<Iter>) {
      static_assert(std::is_same_v<std::decay_t<decltype(*start)>, int32_t>);

      return {&*start, std::distance(start, stop)};
    }
    else {
      static_assert(
          std::is_same_v<std::decay_t<typename std::iterator_traits<Iter>::value_type>, int32_t>);

      buffer.assign(start, stop);
      return {buffer.data(), buffer.size()};
    }
  }


  inline optional_bool to_solve_result(int ipasir2_solve_result)
  {
    switch (ipasir2_solve_result) {
    case 10:
      return optional_bool(true);
    case 20:
      return optional_bool(false);
    default:
      return optional_bool();
    }
  }
}


class solver {
public:
  solver() { detail::throw_if_failed(ipasir2_init(&m_handle)); }


  /// Adds the literals in [start, stop) as a clause to the solver.
  ///
  /// \tparam Iter This type can be one of:
  ///               - iterator type with values convertible to `int32_t`
  ///               - pointer type which is convertible to `int32_t const*`
  ///
  /// \throws `ipasir2_error` if the implementation indicated an error.
  template <typename Iter>
  void add_clause(Iter start, Iter stop, ipasir2_redundancy redundancy = IPASIR2_R_NONE)
  {
    auto const& [clause_ptr, clause_len] = detail::as_contiguous(start, stop, m_clause_buf);
    detail::throw_if_failed(ipasir2_add(m_handle, clause_ptr, clause_len, redundancy));
  }


  /// Adds a clause to the solver.
  ///
  /// For example, this function can be used to add literals stored in a `std::vector<int32_t>`,
  /// or in a custom clause type (see requirements below).
  ///
  /// NB: For custom clause types, performance can be gained when the iterator type satisfies
  /// `std::contiguous_iterator` and C++20 is used. The IPASIR2 wrapper will then directly pass
  /// the buffer to the solver. For C++17 and earlier, the clause is copied unless `LitContainer`
  /// is a `std::vector`, or has pointer-type iterators.
  ///
  /// \tparam LitContainer A type with `begin()` and `end()` methods returning either
  ///                       - iterators with values convertible to `int32_t`
  ///                       - pointers convertible to `int32_t const*`.
  ///
  /// \throws `ipasir2_error` if the implementation indicated an error.
  template <typename LitContainer>
  void add_clause(LitContainer const& container, ipasir2_redundancy redundancy = IPASIR2_R_NONE)
  {
    add_clause(container.begin(), container.end(), redundancy);
  }


  template <typename Iter>
  optional_bool solve(Iter assumptions_start, Iter assumptions_stop)
  {
    auto const& [assumptions_ptr, assumptions_len]
        = detail::as_contiguous(assumptions_start, assumptions_stop, m_clause_buf);
    int result = 0;
    detail::throw_if_failed(ipasir2_solve(m_handle, &result, assumptions_ptr, assumptions_len));
    return detail::to_solve_result(result);
  }


  template <typename LitContainer>
  optional_bool solve(LitContainer const& assumptions)
  {
    return solve(assumptions.begin(), assumptions.end());
  }


  optional_bool solve()
  {
    int32_t result = 0;
    detail::throw_if_failed(ipasir2_solve(m_handle, &result, nullptr, 0));
    return detail::to_solve_result(result);
  }


  optional_bool lit_value(int32_t lit) const
  {
    int32_t result = 0;
    detail::throw_if_failed(ipasir2_val(m_handle, lit, &result));

    if (result == lit) {
      return optional_bool{true};
    }
    else if (result == -lit) {
      return optional_bool{false};
    }
    else if (result == 0) {
      return optional_bool{};
    }
    else {
      throw ipasir2_error{"Unknown truth value received from solver"};
    }
  }


  bool lit_failed(int32_t lit) const
  {
    int32_t result = 0;
    detail::throw_if_failed(ipasir2_failed(m_handle, lit, &result));

    if (result != 0 && result != 1) {
      throw ipasir2_error{"Unknown truth value received from solver"};
    }

    return result == 1;
  }


  ~solver() { ipasir2_release(m_handle); }


  solver(solver const&) = delete;
  solver& operator=(solver const&) = delete;
  solver(solver&&) = delete;
  solver& operator=(solver&&) = delete;

private:
  void* m_handle = nullptr;
  std::vector<int32_t> m_clause_buf;
};


class ipasir2 {
public:
  ipasir2() {}


  std::unique_ptr<solver> create_solver() { return std::make_unique<solver>(); }


  std::string signature() const
  {
    char const* result = nullptr;
    detail::throw_if_failed(ipasir2_signature(&result));
    return result;
  }


  ipasir2(ipasir2 const&) = delete;
  ipasir2& operator=(ipasir2 const&) = delete;
  ipasir2(ipasir2&&) noexcept = default;
  ipasir2& operator=(ipasir2&&) noexcept = default;
};

}
