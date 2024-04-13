// C++ wrapper for IPASIR-2 solvers (current development version)
//
// https://github.com/ipasir2/ipasir2-cpp
//
// Copyright (c) 2024 Felix Kutzner
// This file is subject to the MIT license (https://spdx.org/licenses/MIT.html).
// SPDX-License-Identifier: MIT

/// \file
///
/// Main header of ipasir2-cpp, defining all platform-agnostic functionality.

#pragma once

#if defined(_MSC_VER)
  // MSVC sets __cplusplus to 199711L by default even when newer C++ standards
  // are used (see /Zc:__cplusplus)
  #define IPASIR2_CPP_STANDARD _MSVC_LANG
#else
  #define IPASIR2_CPP_STANDARD __cplusplus
#endif

#if IPASIR2_CPP_STANDARD < 201703L
#error "ipasir2cpp.h requires C++17 or newer"
#endif

#include <ipasir2.h>

#include <array>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if __has_include(<version>)
#include <version>
#endif

#if __cpp_lib_span
#include <span>
#endif


// An inline namespace is used to prevent ODR issues: ipasir2_<major>_<minor>_<patch>[_dev]
// Unfortunately, clang-format's "Inner" namespace formatting rule only supports not indenting
// the outermost namespace, but not the outermost two namespaces. To avoid needless indentation:
#define IPASIR2CPP_BEGIN_INLINE_NAMESPACE inline namespace ipasir2_0_0_0_dev {
#define IPASIR2CPP_END_INLINE_NAMESPACE }


namespace ipasir2 {
IPASIR2CPP_BEGIN_INLINE_NAMESPACE

namespace detail {
  inline std::string get_description(ipasir2_errorcode errorcode)
  {
    switch (errorcode) {
    case IPASIR2_E_OK:
      return "The function call was successful.";
    case IPASIR2_E_UNKNOWN:
      return "The function call failed for an unknown reason.";
    case IPASIR2_E_UNSUPPORTED:
      return "The function is not implemented by the solver.";
    case IPASIR2_E_UNSUPPORTED_ARGUMENT:
      return "The function is not implemented for handling the given argument value.";
    case IPASIR2_E_UNSUPPORTED_OPTION:
      return "The option is not supported by the solver.";
    case IPASIR2_E_INVALID_STATE:
      return "The function call is not allowed in the current state of the solver.";
    case IPASIR2_E_INVALID_ARGUMENT:
      return "The function call failed because of an invalid argument.";
    case IPASIR2_E_INVALID_OPTION_VALUE:
      return "The option value is outside the allowed range.";
    default:
      return "Unknown error";
    }
  }
}


class ipasir2_error : public std::exception {
public:
  explicit ipasir2_error(std::string_view func_name, ipasir2_errorcode code)
    : m_message{std::string{func_name} + "(): " + detail::get_description(code)}
    , m_errorcode{code} {};

  explicit ipasir2_error(std::string_view message) : m_message{message} {};

  char const* what() const noexcept override { return m_message.c_str(); }

  std::optional<ipasir2_errorcode> const& error_code() const noexcept { return m_errorcode; }

private:
  std::string m_message;
  std::optional<ipasir2_errorcode> m_errorcode;
};


/// \brief Restricted version of std::optional<bool>
///
/// In contrast to std::bool, this class does not have implicit
/// conversion to bool, no comparison operators for bool, and no
/// unchecked access to the value.
class optional_bool {
public:
  constexpr explicit optional_bool(bool value) : m_value{value} {}

  /// \brief Constructs an empty optional_bool
  constexpr optional_bool() {}


  /// \brief Returns the contained value.
  ///
  /// \throws std::bad_optional_access if the object has no value.
  constexpr bool unwrap() const { return m_value.value(); }

  template <typename T>
  constexpr T const& map(T const& if_true, T const& if_false, T const& if_empty) const
  {
    if (m_value.has_value()) {
      return *m_value ? if_true : if_false;
    }
    return if_empty;
  }

  constexpr bool has_value() const { return m_value.has_value(); }

  constexpr std::optional<bool> as_std_optional() const { return m_value; }

  constexpr auto operator==(optional_bool const& rhs) const noexcept
  {
    return this == (&rhs) || m_value == rhs.m_value;
  }

  constexpr auto operator!=(optional_bool const& rhs) const noexcept { return !(*this == rhs); }

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


inline std::string to_string(optional_bool const& optbool)
{
  return optbool.map<std::string>("sat", "unsat", "unknown");
}


/// \brief Span-like clause view with implicit conversion to std::span, used for C++17 compatibility
template <typename Lit = int32_t>
class clause_view {
public:
  clause_view(Lit const* start, Lit const* stop) : m_start{start}, m_stop{stop} {}

  explicit clause_view(std::vector<Lit> const& clause)
    : m_start{clause.data()}, m_stop{clause.data() + clause.size()}
  {
  }

  Lit const* begin() const noexcept { return m_start; }
  Lit const* end() const noexcept { return m_stop; }
  bool empty() const noexcept { return m_start == m_stop; }

#if __cpp_lib_span
  operator std::span<Lit const>() const noexcept { return std::span<Lit const>(m_start, m_stop); }
#endif

private:
  Lit const* m_start;
  Lit const* m_stop;
};


/// \brief Type-trait struct for custom literal type converters
///
/// The IPASIR-2 wrapper can automatically convert client literal types `Lit` if
/// this structure is specialized for `Lit` and the specialization has functions
/// with the following signatures:
///
///    static int32_t to_ipasir2_lit(Lit const&);
///    static Lit from_ipasir2_lit(int32_t literal);
///
/// See also example `04_custom_types.cpp`.
template <typename Lit>
struct lit_traits {};


template <>
struct lit_traits<int32_t> {
  static int32_t to_ipasir2_lit(int32_t literal) { return literal; }
  static int32_t from_ipasir2_lit(int32_t literal) { return literal; }
};


class dll {
public:
  dll() {}
  virtual ~dll() {}

  template <typename T>
  void load_func_sym(T& func_ptr, std::string_view name) const
  {
    func_ptr = reinterpret_cast<T>(get_sym(name));
  }


  dll(dll const& rhs) = delete;
  dll& operator=(dll const& rhs) = delete;
  dll(dll&& rhs) = delete;
  dll& operator=(dll&& rhs) = delete;

protected:
  virtual void* get_sym(std::string_view name) const = 0;
};


namespace detail {
  struct shared_c_api {
    decltype(&ipasir2_add) add = nullptr;
    decltype(&ipasir2_failed) failed = nullptr;
    decltype(&ipasir2_init) init = nullptr;
    decltype(&ipasir2_options) options = nullptr;
    decltype(&ipasir2_release) release = nullptr;
    decltype(&ipasir2_set_export) set_export = nullptr;
    decltype(&ipasir2_set_option) set_option = nullptr;
    decltype(&ipasir2_set_terminate) set_terminate = nullptr;
    decltype(&ipasir2_signature) signature = nullptr;
    decltype(&ipasir2_solve) solve = nullptr;
    decltype(&ipasir2_val) val = nullptr;
    std::shared_ptr<dll const> m_dll;
  };


  inline void throw_if_failed(ipasir2_errorcode errorcode, std::string_view func_name)
  {
    if (errorcode != IPASIR2_E_OK) {
      throw ipasir2_error{func_name, errorcode};
    }
  }


  /// \brief Metafunction that determines whether the given type is a literal type.
  ///
  /// \internal
  ///
  /// A literal type is either int32_t, or a type T for which `lit_traits<T>`
  /// is specialized with conversion functions. With the latter, clients can
  /// use custom literal types with the IPASIR-2 wrapper.
  template <typename T, typename = void>
  struct is_literal : public std::false_type {};


  template <typename T>
  struct is_literal<
      T,
      std::void_t<decltype(lit_traits<std::decay_t<T>>::to_ipasir2_lit(std::declval<T>()))>>
    : public std::true_type {};


  template <typename T>
  constexpr bool is_literal_v = is_literal<T>::value;


  template <typename... Ts>
  using enable_if_all_literals_t = std::enable_if_t<std::conjunction_v<is_literal<Ts>...>>;


  template <typename T>
  using enable_unless_literal_t = std::enable_if_t<!is_literal<T>::value>;


  // std::begin and std::end are imported into the detail namespace to support ADL lookup of
  // begin() and end() of custom clause types in enable_if_lit_container_t below.
  using std::begin;
  using std::end;

  // Unfortunately, this currently doesn't support C-style arrays with known size.
  // std::begin and std::end have specializations for them, but they can't be found
  // via ADL. However, using C-style arrays should be a niche use case, especially
  // since std::array can be used instead.
  template <typename T>
  using enable_if_lit_container_t
      = std::enable_if_t<std::conjunction_v<is_literal<decltype(*begin(std::declval<T>()))>,
                                            is_literal<decltype(*end(std::declval<T>()))>>>;


#if __cpp_lib_concepts
  template <typename T>
  constexpr bool is_contiguous_int32_iter
      = std::contiguous_iterator<T>
        && std::is_same_v<std::decay_t<decltype(*std::declval<T>())>, int32_t>;
#else
  // In C++17, there is no programmatic way of checking whether an iterator is contiguous,
  // so only a few common cases are covered:
  // clang-format off
  template <typename T>
  constexpr bool is_contiguous_int32_iter
      = std::is_same_v<std::decay_t<T>, int32_t*>
        || std::is_same_v<std::decay_t<T>, int32_t const*>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::iterator>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::const_iterator>;
  // clang-format on
#endif


  /// \brief Function for getting contiguous access to literals, buffering/converting literals as needed
  ///
  /// \internal
  ///
  /// \returns pointer to the array of int32_t literals, and the size of the array
  template <typename Iter>
  std::pair<int32_t const*, size_t>
  as_contiguous_int32s(Iter start, Iter stop, std::vector<int32_t>& buffer)
  {
    if (start == stop) {
      buffer.clear();
      buffer.push_back(0);
      return {buffer.data(), 0};
    }

    if constexpr (detail::is_contiguous_int32_iter<Iter>) {
      // In this case, the data is already placed in a buffer of int32_t, which can be directly used:
      return {&*start, std::distance(start, stop)};
    }
    else if constexpr (std::is_same_v<typename std::iterator_traits<Iter>::value_type, int32_t>) {
      // Here buffering is needed since the literals are not in contiguous memory:
      buffer.assign(start, stop);
      return {buffer.data(), buffer.size()};
    }
    else {
      // In this case the literal type is not int32_t. Conversion is needed, and hence also buffering:
      using lit_type = std::decay_t<typename std::iterator_traits<Iter>::value_type>;
      buffer.clear();
      for (auto iter = start; iter != stop; ++iter) {
        buffer.push_back(lit_traits<lit_type>::to_ipasir2_lit(*iter));
      }
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


  inline clause_view<int32_t> clause_view_from_zero_terminated(int32_t const* start)
  {
    int32_t const* iter = start;
    while (*iter != 0) {
      ++iter;
    }
    return clause_view{start, iter};
  }
}


enum class redundancy {
  none = IPASIR2_R_NONE,
  forgettable = IPASIR2_R_FORGETTABLE,
  equisatisfiable = IPASIR2_R_EQUISATISFIABLE,
  equivalent = IPASIR2_R_EQUIVALENT
};


class option {
public:
  std::string_view name() const { return m_solver_handle->name; }
  int64_t min_value() const { return m_solver_handle->min; }
  int64_t max_value() const { return m_solver_handle->max; }
  bool is_tunable() const { return m_solver_handle->tunable != 0; }
  bool is_indexed() const { return m_solver_handle->indexed != 0; }

private:
  explicit option(ipasir2_option const& solver_handle) : m_solver_handle{&solver_handle} {}

  friend class solver;
  ipasir2_option const* m_solver_handle;
};


/// \brief Class representing an IPASIR-2 solver instance.
///
/// Objects of this class can neither be moved nor copied.
class solver {
public:
  /// \brief Adds the literals in [start, stop) as a clause to the solver.
  ///
  /// \tparam Iter Iterator over literals, or a pointer to a literal type (e.g. `int32_t const*`)
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Iter, typename = detail::enable_unless_literal_t<Iter>>
  void add(Iter start, Iter stop, redundancy red = redundancy::none)
  {
    auto const& [clause_ptr, clause_len] = detail::as_contiguous_int32s(start, stop, m_clause_buf);
    ipasir2_redundancy c_redundancy = static_cast<ipasir2_redundancy>(red);
    detail::throw_if_failed(m_api.add(m_solver.get(), clause_ptr, clause_len, c_redundancy),
                            "ipasir2_add");
  }


  /// \brief Adds a clause to the solver.
  ///
  /// For example, this function can be used to add literals stored in a `std::vector<int32_t>`,
  /// or in a custom clause type (see requirements below).
  ///
  /// \tparam LitContainer A type for which `begin()` and `end()` functions return iterators over
  ///                      literals, or pointers to a contigous buffer of literals.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename LitContainer, typename = detail::enable_if_lit_container_t<LitContainer>>
  void add(LitContainer const& container, redundancy red = redundancy::none)
  {
    using std::begin;
    using std::end;
    add(begin(container), end(container), red);
  }


private:
  template <typename... lits_and_redundancy, size_t... lit_indices>
  void add_parampack_helper(std::tuple<lits_and_redundancy...> params,
                            std::index_sequence<lit_indices...>)
  {
    std::array literals_array{std::get<lit_indices>(params)...};
    redundancy red = std::get<sizeof...(lits_and_redundancy) - 1>(params);
    add(literals_array.begin(), literals_array.end(), red);
  }


public:
  /// \brief Adds a clause to the solver.
  ///
  /// The arguments of this functions are literals, optionally followed
  /// by a single `redundancy` argument.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename T, typename... Ts, typename = std::enable_if_t<detail::is_literal_v<T>>>
  void add(T lit, Ts... rest)
  {
    if constexpr (std::conjunction_v<detail::is_literal<Ts>...>) {
      std::array literals_array{lit, rest...};
      add(literals_array.begin(), literals_array.end(), redundancy::none);
    }
    else {
      // Here we are in a pickle: this method optionally supports specifying
      // the clause redundancy as its last argument. However, this can't be
      // implemented as an optional last parameter, due to the parameter pack.
      // This is solved by a trick: create a tuple of all arguments, then
      // use an index sequence to std::get<>() the literals.
      add_parampack_helper(std::forward_as_tuple(lit, rest...),
                           std::make_index_sequence<sizeof...(Ts)>{});
    }
  }


  /// \brief Checks if the problem is satisfiable under the given assumptions.
  ///
  /// \returns If the solver produced a result, a boolean value is returned representing
  ///          the satisfiability of the problem instance. Otherwise, nothing is returned.
  ///
  /// \tparam Iter Iterator over literals, or a pointer to a literal type (e.g. `int32_t const*`)
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Iter, typename = detail::enable_unless_literal_t<Iter>>
  optional_bool solve(Iter assumptions_start, Iter assumptions_stop)
  {
    auto const& [assumptions_ptr, assumptions_len]
        = detail::as_contiguous_int32s(assumptions_start, assumptions_stop, m_clause_buf);
    int result = 0;

    ipasir2_errorcode const solve_status
        = m_api.solve(m_solver.get(), &result, assumptions_ptr, assumptions_len);
    rethrow_exception_thrown_in_callback();

    detail::throw_if_failed(solve_status, "ipasir2_solve");
    return detail::to_solve_result(result);
  }


  /// \brief Checks if the problem instance is satisfiable under the given assumptions.
  ///
  /// \returns If the solver produced a result, a boolean value is returned representing
  ///          the satisfiability of the problem instance. Otherwise, nothing is returned.
  ///
  /// \tparam LitContainer A type for which `begin()` and `end()` functions return iterators over
  ///                      literals, or pointers to a contigous buffer of literals.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename LitContainer, typename = detail::enable_if_lit_container_t<LitContainer>>
  optional_bool solve(LitContainer const& assumptions)
  {
    using std::begin;
    using std::end;
    return solve(begin(assumptions), end(assumptions));
  }


  /// \brief Checks if the problem is satisfiable under the given assumptions.
  ///
  /// \returns If the solver produced a result, a boolean value is returned representing
  ///          the satisfiability of the problem instance. Otherwise, nothing is returned.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename T,
            typename... Ts,
            typename = std::void_t<detail::is_literal<T>, detail::enable_if_all_literals_t<Ts...>>>
  optional_bool solve(T assumption, Ts... assumptions_rest)
  {
    std::array assumptions{assumption, assumptions_rest...};
    return solve(assumptions);
  }


  /// \brief Checks if the problem is satisfiable.
  ///
  /// \returns If the solver produced a result, a boolean value is returned representing
  ///          the satisfiability of the problem instance. Otherwise, nothing is returned.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  optional_bool solve()
  {
    int32_t result = 0;
    ipasir2_errorcode const solve_status = m_api.solve(m_solver.get(), &result, nullptr, 0);

    rethrow_exception_thrown_in_callback();
    detail::throw_if_failed(solve_status, "ipasir2_solve");

    return detail::to_solve_result(result);
  }


  /// \brief Returns the literal's value in the current assignment.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Lit>
  optional_bool lit_value(Lit lit) const
  {
    int32_t const ipasir2_lit = lit_traits<Lit>::to_ipasir2_lit(lit);
    int32_t result = 0;
    detail::throw_if_failed(m_api.val(m_solver.get(), ipasir2_lit, &result), "ipasir2_val");

    if (result == ipasir2_lit) {
      return optional_bool{true};
    }
    else if (result == -ipasir2_lit) {
      return optional_bool{false};
    }
    else if (result == 0) {
      return optional_bool{};
    }
    else {
      throw ipasir2_error{"Unknown truth value received from solver"};
    }
  }


  /// \brief Checks if given assumption literal was used to prove the unsatisfiability
  ///        in the last invocation of solve().
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Lit>
  bool assumption_failed(Lit lit) const
  {
    int32_t const ipasir2_lit = lit_traits<Lit>::to_ipasir2_lit(lit);
    int32_t result = 0;
    detail::throw_if_failed(m_api.failed(m_solver.get(), ipasir2_lit, &result), "ipasir2_failed");

    if (result != 0 && result != 1) {
      throw ipasir2_error{"Unknown truth value received from solver"};
    }

    return result == 1;
  }


  /// \brief Sets a callback function for aborting the solve process.
  ///
  /// The solver calls this function regularly during solve(). If it returns `true`,
  /// the SAT search is aborted.
  ///
  /// If an exception is thrown from the callback, no further callbacks are invoked until
  /// solve() has finished. The exception is rethrown from solve().
  ///
  /// \tparam Func A callable without parameters that returns a value testable as a `bool`.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Func>
  void set_terminate_callback(Func&& callback)
  {
    if (!m_terminate_callback) {
      auto result = m_api.set_terminate(m_solver.get(), this, [](void* data) -> int {
        solver* s = reinterpret_cast<solver*>(data);

        if (s->m_exception_thrown_in_callback) {
          // The client-provided callback might throw another exception, so don't call it.
          // Since the exception is rethrown from solve(), solving is aborted:
          return true;
        }

        try {
          if (s->m_terminate_callback) {
            return s->m_terminate_callback();
          }
          else {
            // Clearing the callback has failed ~> behave as if user hadn't set a callback
            return false;
          }
        }
        catch (...) {
          // Can't throw through the C API ~> exception is rethrown from solve()
          s->m_exception_thrown_in_callback = std::current_exception();
          return true;
        }
      });
      detail::throw_if_failed(result, "ipasir2_set_terminate");
    }

    m_terminate_callback = callback;
  }


  /// \brief Disables the callback set via `set_terminate_callback()`.
  void clear_terminate_callback()
  {
    m_terminate_callback = {};
    detail::throw_if_failed(m_api.set_terminate(m_solver.get(), nullptr, nullptr),
                            "ipasir2_set_terminate");
  }


  /// \brief Sets a callback for observing learnt clauses.
  ///
  /// The solver calls this function during solve() for all learnt clauses of size `max_size` or less.
  ///
  /// If an exception is thrown from the callback, no further callbacks are invoked until
  /// solve() has finished. The exception is rethrown from solve().
  ///
  /// \tparam Func A callable with parameter `ipasir2::clause_view<Lit>` or `std::span<Lit const>` for
  ///              any literal type Lit (e.g. int32_t).
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Lit = int32_t, typename Func>
  void set_export_callback(Func&& callback, size_t max_size = 0)
  {
    // Reset the callback function so the old callback is not called anymore in case the IPASIR call fails
    m_export_callback = {};

    auto result
        = m_api.set_export(m_solver.get(), this, max_size, [](void* data, int32_t const* clause) {
            solver* s = reinterpret_cast<solver*>(data);
            if (s->m_exception_thrown_in_callback) {
              // The client-provided callback might throw another exception, so don't call it.
              return;
            }

            try {
              if (s->m_export_callback) {
                return s->m_export_callback(detail::clause_view_from_zero_terminated(clause));
              }
            }
            catch (...) {
              // Can't throw through the C API ~> exception is rethrown from solve()
              s->m_exception_thrown_in_callback = std::current_exception();
            }
          });
    detail::throw_if_failed(result, "ipasir2_set_export");

    if constexpr (std::is_same_v<std::decay_t<Lit>, int32_t>) {
      m_export_callback = callback;
    }
    else {
      m_export_callback
          = [callback, buf = std::vector<Lit>{}](clause_view<int32_t> native_clause) mutable {
              buf.clear();
              for (int32_t lit : native_clause) {
                buf.push_back(lit_traits<std::decay_t<Lit>>::from_ipasir2_lit(lit));
              }
              callback(clause_view<Lit>{buf});
            };
    }
  }


  /// \brief Disables the callback set via `set_export_callback()`.
  void clear_export_callback()
  {
    m_export_callback = {};
    detail::throw_if_failed(m_api.set_export(m_solver.get(), nullptr, 0, nullptr),
                            "ipasir2_set_export");
  }


  option get_option(std::string_view name) const
  {
    ensure_options_cached();

    auto result = std::find_if(m_cached_options->begin(),
                               m_cached_options->end(),
                               [&](option const& candidate) { return candidate.name() == name; });

    if (result == m_cached_options->end()) {
      throw ipasir2_error{"the solver does not implement the given option"};
    }

    return *result;
  }


  std::vector<option> const& get_options() const
  {
    ensure_options_cached();

    return *m_cached_options;
  }


  bool has_option(std::string_view name) const
  {
    ensure_options_cached();

    return std::find_if(m_cached_options->begin(),
                        m_cached_options->end(),
                        [&](option const& candidate) { return candidate.name() == name; })
           != m_cached_options->end();
  }


  void set_option(option const& option, int64_t value, int64_t index = 0)
  {
    detail::throw_if_failed(m_api.set_option(m_solver.get(), option.m_solver_handle, value, index),
                            "ipasir2_set_option");
  }


  void set_option(std::string_view name, int64_t value, int64_t index = 0)
  {
    set_option(get_option(name), value, index);
  }


  /// \brief Returns the IPASIR-2 solver handle.
  ///
  /// The handle is valid for the lifetime of the `solver` object.
  /// This function can be used to access non-standard extensions of the IPASIR-2 API.
  void* get_ipasir2_handle() { return m_solver.get(); }


  // `solver` objects manage IPASIR2 resources. Also, pointers to `solver` objects
  // are passed to IPASIR2 solvers as cookies for callbacks. Hence, both copy and
  // move operators are deleted for `solver`.
  solver(solver const&) = delete;
  solver& operator=(solver const&) = delete;
  solver(solver&&) = delete;
  solver& operator=(solver&&) = delete;


private:
  friend class ipasir2;

  static std::unique_ptr<solver> create(detail::shared_c_api const& api)
  {
    // std::make_unique can't be used here, since the constructor is private
    return std::unique_ptr<solver>{new solver(api)};
  }


  explicit solver(detail::shared_c_api const& api) : m_api{api}, m_solver{nullptr, nullptr}
  {
    void* handle = nullptr;
    detail::throw_if_failed(m_api.init(&handle), "ipasir2_init");
    m_solver = unique_ipasir2_solver{handle, m_api.release};
  }


  void ensure_options_cached() const
  {
    if (!m_cached_options.has_value()) {
      ipasir2_option const* option_cursor = nullptr;
      detail::throw_if_failed(m_api.options(m_solver.get(), &option_cursor), "ipasir2_options");

      m_cached_options = std::vector<option>{};

      while (option_cursor->name != nullptr) {
        m_cached_options->push_back(option{*option_cursor});
        ++option_cursor;
      }
    }
  }


  void rethrow_exception_thrown_in_callback()
  {
    if (m_exception_thrown_in_callback) {
      std::exception_ptr to_throw = m_exception_thrown_in_callback;
      m_exception_thrown_in_callback = nullptr;
      std::rethrow_exception(to_throw);
    }
  }


  detail::shared_c_api m_api;

  using unique_ipasir2_solver = std::unique_ptr<void, decltype(&ipasir2_release)>;
  unique_ipasir2_solver m_solver;

  std::vector<int32_t> m_clause_buf;

  std::function<bool()> m_terminate_callback;
  std::function<void(clause_view<int32_t>)> m_export_callback;

  // If an exception is thrown in a client-supplied callback function, it is stored
  // and rethrown from solve(). This is required since we can't throw C++ exceptions
  // across the IPASIR-2 API. An alternative would be to require callback functions
  // to be noexcept, but that would make using the callbacks needlessly errorprone.
  std::exception_ptr m_exception_thrown_in_callback;

  mutable std::optional<std::vector<option>> m_cached_options;
};


/// \brief Class representing an IPASIR-2 implementation.
///
/// Objects of this class can be used to create solver instances, and
/// to call IPASIR-2 functions that are not tied to a solver instance.
class ipasir2 {
public:
  /// \brief Creates a solver instance.
  ///
  /// The lifetime of the created solver instance may exceed the lifetime of the `ipasir2`
  /// object used to create it.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  std::unique_ptr<solver> create_solver() { return solver::create(m_api); }


  /// \brief Returns the name and the version of the IPASIR-2 implementation.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  std::string signature() const
  {
    char const* result = nullptr;
    detail::throw_if_failed(m_api.signature(&result), "ipasir2_signature");
    return result;
  }


  ipasir2(ipasir2 const&) = delete;
  ipasir2& operator=(ipasir2 const&) = delete;
  ipasir2(ipasir2&&) noexcept = default;
  ipasir2& operator=(ipasir2&&) noexcept = default;


  explicit ipasir2(detail::shared_c_api const& api) : m_api{api} {}

private:
  detail::shared_c_api m_api;
};


/// \brief Creates an `ipasir2` object using an IPASIR-2 implementation linked at build time.
template <typename = void /* prevent instantiation unless called */>
ipasir2 create_api()
{
  // Ideally, this would just be the default constructor of `ipasir2`.
  // However, clients then would get linker errors if a `ipasir2` object
  // is accidentally default-constructed and they don't link to IPASIR2
  // at build time.

  detail::shared_c_api ipasir2_funcs;
  ipasir2_funcs.add = &ipasir2_add;
  ipasir2_funcs.failed = &ipasir2_failed;
  ipasir2_funcs.init = &ipasir2_init;
  ipasir2_funcs.options = &ipasir2_options;
  ipasir2_funcs.release = &ipasir2_release;
  ipasir2_funcs.set_export = &ipasir2_set_export;
  ipasir2_funcs.set_option = &ipasir2_set_option;
  ipasir2_funcs.set_terminate = &ipasir2_set_terminate;
  ipasir2_funcs.signature = &ipasir2_signature;
  ipasir2_funcs.solve = &ipasir2_solve;
  ipasir2_funcs.val = &ipasir2_val;

  return ipasir2{ipasir2_funcs};
}


/// \brief Creates an `ipasir2` object using an IPASIR-2 implementation selected at runtime.
///
/// This overload does not load the IPASIR-2 library, but uses the abstract `dll` interface.
/// An overload taking a std::filesystem::path to an IPASIR-2 library is provided in
/// `ipasir2cpp_dl.h`.
///
/// \throws `ipasir2_error` if IPASIR-2 symbols are missing in the library.
inline ipasir2 create_api(std::shared_ptr<dll const> dll)
{
  detail::shared_c_api ipasir2_funcs;

  ipasir2_funcs.m_dll = dll;
  dll->load_func_sym(ipasir2_funcs.add, "ipasir2_add");
  dll->load_func_sym(ipasir2_funcs.failed, "ipasir2_failed");
  dll->load_func_sym(ipasir2_funcs.init, "ipasir2_init");
  dll->load_func_sym(ipasir2_funcs.options, "ipasir2_options");
  dll->load_func_sym(ipasir2_funcs.release, "ipasir2_release");
  dll->load_func_sym(ipasir2_funcs.set_export, "ipasir2_set_export");
  dll->load_func_sym(ipasir2_funcs.set_option, "ipasir2_set_option");
  dll->load_func_sym(ipasir2_funcs.set_terminate, "ipasir2_set_terminate");
  dll->load_func_sym(ipasir2_funcs.signature, "ipasir2_signature");
  dll->load_func_sym(ipasir2_funcs.solve, "ipasir2_solve");
  dll->load_func_sym(ipasir2_funcs.val, "ipasir2_val");

  return ipasir2{ipasir2_funcs};
}

IPASIR2CPP_END_INLINE_NAMESPACE
}
