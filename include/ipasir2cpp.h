// C++ wrapper for IPASIR-2 solvers
//
// Copyright (c) 2024 Felix Kutzner
// This file is subject to the MIT license (https://spdx.org/licenses/MIT.html).
// SPDX-License-Identifier: MIT

/// \file

#pragma once

#if __cplusplus < 201703L
#error "ipasir2cpp.h requires C++17 or newer"
#endif

#include <ipasir2.h>

#include <exception>
#include <filesystem>
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

#if __has_include(<dlfcn.h>)
#include <dlfcn.h>
#define IPASIR2CPP_HAS_DLOPEN
#endif


namespace ipasir2 {

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


namespace detail {
  class shared_c_api {
  public:
#if defined(IPASIR2CPP_HAS_DLOPEN)
    static shared_c_api load(std::filesystem::path const& shared_lib)
    {
      shared_c_api result;
      load_lib(shared_lib, result);
      load_sym(result.add, "ipasir2_add", result);
      load_sym(result.failed, "ipasir2_failed", result);
      load_sym(result.init, "ipasir2_init", result);
      load_sym(result.options, "ipasir2_options", result);
      load_sym(result.release, "ipasir2_release", result);
      load_sym(result.set_export, "ipasir2_set_export", result);
      load_sym(result.set_option, "ipasir2_set_option", result);
      load_sym(result.set_terminate, "ipasir2_set_terminate", result);
      load_sym(result.signature, "ipasir2_signature", result);
      load_sym(result.solve, "ipasir2_solve", result);
      load_sym(result.val, "ipasir2_val", result);
      return result;
    }
#endif


    template <typename = void /* prevent instantiation unless called */>
    static shared_c_api with_linked_syms()
    {
      shared_c_api result;
      result.add = &ipasir2_add;
      result.failed = &ipasir2_failed;
      result.init = &ipasir2_init;
      result.options = &ipasir2_options;
      result.release = &ipasir2_release;
      result.set_export = &ipasir2_set_export;
      result.set_option = &ipasir2_set_option;
      result.set_terminate = &ipasir2_set_terminate;
      result.signature = &ipasir2_signature;
      result.solve = &ipasir2_solve;
      result.val = &ipasir2_val;
      return result;
    }


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


  private:
    // Unlike IPASIR-2 solver handles, the shared-library handle is not accessible
    // since it is an implementation detail, not an IPASIR-2 item.
    std::shared_ptr<void> m_lib_handle;


#if defined(IPASIR2CPP_HAS_DLOPEN)
    static void load_lib(std::filesystem::path const& path, shared_c_api& api)
    {
      api.m_lib_handle = std::shared_ptr<void>(dlopen(path.c_str(), RTLD_NOW), [](void* lib) {
        if (lib != nullptr) {
          dlclose(lib);
        }
      });

      if (api.m_lib_handle == nullptr) {
        throw ipasir2_error{"Could not open the IPASIR2 library."};
      }
    }

    template <typename F>
    static void load_sym(F& func_ptr, std::string_view name, shared_c_api& api)
    {
      func_ptr = reinterpret_cast<F>(dlsym(api.m_lib_handle.get(), name.data()));

      if (func_ptr == nullptr) {
        throw ipasir2_error{"Missing symbol in the IPASIR2 library."};
      }
    }
#endif
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
  /// \tparam Iter This type can be one of:
  ///               - iterator type with values convertible to `int32_t`
  ///               - pointer type which is convertible to `int32_t const*`
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Iter, typename = detail::enable_unless_literal_t<Iter>>
  void add(Iter start, Iter stop, redundancy red = redundancy::none)
  {
    auto const& [clause_ptr, clause_len] = detail::as_contiguous_int32s(start, stop, m_clause_buf);
    ipasir2_redundancy c_redundancy = static_cast<ipasir2_redundancy>(red);
    detail::throw_if_failed(m_api.add(m_handle.get(), clause_ptr, clause_len, c_redundancy),
                            "ipasir2_add");
  }


  /// \brief Adds a clause to the solver.
  ///
  /// For example, this function can be used to add literals stored in a `std::vector<int32_t>`,
  /// or in a custom clause type (see requirements below).
  ///
  /// NB: For custom clause types, performance can be gained when the iterator type satisfies
  /// `std::contiguous_iterator` and C++20 is used. The IPASIR2 wrapper will then directly pass
  /// the buffer to the solver. For C++17 and earlier, the clause is copied unless `LitContainer`
  /// is a `std::vector`, or has pointer-type iterators.
  ///
  /// \tparam LitContainer A type for which `begin()` and `end()` functions return either
  ///                       - iterators with values convertible to `int32_t`
  ///                       - pointers convertible to `int32_t const*`.
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
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Iter, typename = detail::enable_unless_literal_t<Iter>>
  optional_bool solve(Iter assumptions_start, Iter assumptions_stop)
  {
    auto const& [assumptions_ptr, assumptions_len]
        = detail::as_contiguous_int32s(assumptions_start, assumptions_stop, m_clause_buf);
    int result = 0;

    ipasir2_errorcode const solve_status
        = m_api.solve(m_handle.get(), &result, assumptions_ptr, assumptions_len);
    rethrow_exception_thrown_in_callback();

    detail::throw_if_failed(solve_status, "ipasir2_solve");
    return detail::to_solve_result(result);
  }


  /// \brief Checks if the problem instance is satisfiable under the given assumptions.
  ///
  /// \returns If the solver produced a result, a boolean value is returned representing
  ///          the satisfiability of the problem instance. Otherwise, nothing is returned.
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
    ipasir2_errorcode const solve_status = m_api.solve(m_handle.get(), &result, nullptr, 0);

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
    detail::throw_if_failed(m_api.val(m_handle.get(), ipasir2_lit, &result), "ipasir2_val");

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
    detail::throw_if_failed(m_api.failed(m_handle.get(), ipasir2_lit, &result), "ipasir2_failed");

    if (result != 0 && result != 1) {
      throw ipasir2_error{"Unknown truth value received from solver"};
    }

    return result == 1;
  }


  template <typename Func>
  void set_terminate_callback(Func&& callback)
  {
    if (!m_terminate_callback) {
      auto result = m_api.set_terminate(m_handle.get(), this, [](void* data) -> int {
        solver* s = reinterpret_cast<solver*>(data);

        if (s->m_exception_thrown_in_callback) {
          // An exception has previously been thrown from a callback, so abort as fast as possible
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


  void clear_terminate_callback()
  {
    m_terminate_callback = {};
    detail::throw_if_failed(m_api.set_terminate(m_handle.get(), nullptr, nullptr),
                            "ipasir2_set_terminate");
  }


  template <typename Lit = int32_t, typename Func>
  void set_export_callback(Func&& callback, size_t max_size = 0)
  {
    // Reset the callback function so the old callback is not called anymore in case the IPASIR call fails
    m_export_callback = {};

    auto result
        = m_api.set_export(m_handle.get(), this, max_size, [](void* data, int32_t const* clause) {
            solver* s = reinterpret_cast<solver*>(data);
            if (s->m_exception_thrown_in_callback) {
              // An exception has previously been thrown from a callback, so abort as fast as possible
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

    if constexpr (std::is_same_v<Lit, int32_t>) {
      m_export_callback = callback;
    }
    else {
      m_export_callback = [callback](clause_view<int32_t> native_clause) {
        // TODO: reuse buffer?
        std::vector<Lit> buf;
        for (int32_t lit : native_clause) {
          buf.push_back(lit_traits<std::decay_t<Lit>>::from_ipasir2_lit(lit));
        }
        callback(clause_view<Lit>(buf.data(), buf.data() + buf.size()));
      };
    }
  }


  void clear_export_callback()
  {
    m_export_callback = {};
    detail::throw_if_failed(m_api.set_export(m_handle.get(), nullptr, 0, nullptr),
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
    detail::throw_if_failed(m_api.set_option(m_handle.get(), option.m_solver_handle, value, index),
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
  void* get_ipasir2_handle() { return m_handle.get(); }


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


  explicit solver(detail::shared_c_api const& api) : m_api{api}, m_handle{nullptr, nullptr}
  {
    void* handle = nullptr;
    detail::throw_if_failed(m_api.init(&handle), "ipasir2_init");
    m_handle = unique_ipasir2_handle{handle, m_api.release};
  }


  void ensure_options_cached() const
  {
    if (!m_cached_options.has_value()) {
      ipasir2_option const* option_cursor = nullptr;
      detail::throw_if_failed(m_api.options(m_handle.get(), &option_cursor), "ipasir2_options");

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

  using unique_ipasir2_handle = std::unique_ptr<void, decltype(&ipasir2_release)>;
  unique_ipasir2_handle m_handle;
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
  template <typename = void /* prevent instantiation unless called */>
  static ipasir2 create()
  {
    // Ideally, this would just be the default constructor of `ipasir2`.
    // However, clients then would get linker errors if a `ipasir2` object
    // is accidentally default-constructed and they don't link to IPASIR2
    // at build time.
    return ipasir2{detail::shared_c_api::with_linked_syms()};
  }


  template <typename = void /* prevent instantiation unless called */>
  static ipasir2 create(std::filesystem::path const& shared_library)
  {
#if defined(IPASIR2CPP_HAS_DLOPEN)
    return ipasir2{detail::shared_c_api::load(shared_library)};
#else
    static_assert("ipasir2cpp.h does not support loading shared libraries at runtime on this "
                  "platform yet. See the documentation of ipasir2::create_from_api_struct() for a "
                  "workaround.");
#endif
  }


  static ipasir2 create_from_api_struct(detail::shared_c_api api) { return ipasir2{api}; }


  std::unique_ptr<solver> create_solver() { return solver::create(m_api); }


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

private:
  explicit ipasir2(detail::shared_c_api const& api) : m_api{api} {}

  detail::shared_c_api m_api;
};
}
