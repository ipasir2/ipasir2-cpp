// C++ wrapper for IPASIR-2 solvers
//
// Copyright (c) 2024 Felix Kutzner
// This file is subject to the MIT license (https://spdx.org/licenses/MIT.html).
// SPDX-License-Identifier: MIT

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

#if defined(_POSIX_C_SOURCE) && __has_include(<dlfcn.h>)
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


inline std::string to_string(optional_bool const& optbool)
{
  if (optbool.has_value()) {
    return optbool.unwrap() ? "sat" : "unsat";
  }
  return "unknown";
}


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
      load_sym(result.release, "ipasir2_release", result);
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
      result.release = &ipasir2_release;
      result.set_terminate = &ipasir2_set_terminate;
      result.signature = &ipasir2_signature;
      result.solve = &ipasir2_solve;
      result.val = &ipasir2_val;
      return result;
    }


    decltype(&ipasir2_add) add = nullptr;
    decltype(&ipasir2_failed) failed = nullptr;
    decltype(&ipasir2_init) init = nullptr;
    decltype(&ipasir2_release) release = nullptr;
    decltype(&ipasir2_set_terminate) set_terminate = nullptr;
    decltype(&ipasir2_signature) signature = nullptr;
    decltype(&ipasir2_solve) solve = nullptr;
    decltype(&ipasir2_val) val = nullptr;


  private:
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


  template <typename... Ts>
  using enable_if_all_integral_t = std::enable_if_t<std::conjunction_v<std::is_integral<Ts>...>>;


  template <typename T>
  using enable_unless_integral_t = std::enable_if_t<!std::is_integral_v<T>>;


  // TODO: only enable if the container actually contains literals
  // TODO: support ADL lookup of cbegin and cend
  template <typename T>
  using enable_if_lit_container_t = std::void_t<decltype(std::cbegin(std::declval<T>())),
                                                decltype(std::cend(std::declval<T>()))>;


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


enum class redundancy {
  none = IPASIR2_R_NONE,
  forgettable = IPASIR2_R_FORGETTABLE,
  equisatisfiable = IPASIR2_R_EQUISATISFIABLE,
  equivalent = IPASIR2_R_EQUIVALENT
};


class solver {
public:
  /// Adds the literals in [start, stop) as a clause to the solver.
  ///
  /// \tparam Iter This type can be one of:
  ///               - iterator type with values convertible to `int32_t`
  ///               - pointer type which is convertible to `int32_t const*`
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename Iter, typename = detail::enable_unless_integral_t<Iter>>
  void add(Iter start, Iter stop, redundancy red = redundancy::none)
  {
    auto const& [clause_ptr, clause_len] = detail::as_contiguous(start, stop, m_clause_buf);
    ipasir2_redundancy c_redundancy = static_cast<ipasir2_redundancy>(red);
    detail::throw_if_failed(m_api.add(m_handle.get(), clause_ptr, clause_len, c_redundancy),
                            "ipasir2_add");
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
  /// \tparam LitContainer A type for which `std::cbegin()` and `std::cend()` functions return either
  ///                       - iterators with values convertible to `int32_t`
  ///                       - pointers convertible to `int32_t const*`.
  ///
  /// \throws `ipasir2_error` if the underlying IPASIR2 implementation indicated an error.
  template <typename LitContainer, typename = detail::enable_if_lit_container_t<LitContainer>>
  void add(LitContainer const& container, redundancy red = redundancy::none)
  {
    add(std::cbegin(container), std::cend(container), red);
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
  template <typename... Ts>
  void add(int32_t lit, Ts... rest)
  {
    if constexpr (std::conjunction_v<std::is_integral<Ts>...>) {
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


  template <typename Iter, typename = detail::enable_unless_integral_t<Iter>>
  optional_bool solve(Iter assumptions_start, Iter assumptions_stop)
  {
    auto const& [assumptions_ptr, assumptions_len]
        = detail::as_contiguous(assumptions_start, assumptions_stop, m_clause_buf);
    int result = 0;
    detail::throw_if_failed(m_api.solve(m_handle.get(), &result, assumptions_ptr, assumptions_len),
                            "ipasir2_solve");
    return detail::to_solve_result(result);
  }


  template <typename LitContainer, typename = detail::enable_if_lit_container_t<LitContainer>>
  optional_bool solve(LitContainer const& assumptions)
  {
    return solve(std::cbegin(assumptions), std::cend(assumptions));
  }


  template <typename... Ints, typename = detail::enable_if_all_integral_t<Ints...>>
  optional_bool solve(int32_t assumption, Ints... rest)
  {
    std::array assumptions{assumption, rest...};
    return solve(assumptions);
  }


  optional_bool solve()
  {
    int32_t result = 0;
    detail::throw_if_failed(m_api.solve(m_handle.get(), &result, nullptr, 0), "ipasir2_solve");
    return detail::to_solve_result(result);
  }


  optional_bool lit_value(int32_t lit) const
  {
    int32_t result = 0;
    detail::throw_if_failed(m_api.val(m_handle.get(), lit, &result), "ipasir2_val");

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
    detail::throw_if_failed(m_api.failed(m_handle.get(), lit, &result), "ipasir2_failed");

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
        if (s->m_terminate_callback) {
          return s->m_terminate_callback();
        }
        else {
          // Clearing the callback has failed ~> behave as if user hadn't set a callback
          return false;
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

  detail::shared_c_api m_api;

  using unique_ipasir2_handle = std::unique_ptr<void, decltype(&ipasir2_release)>;
  unique_ipasir2_handle m_handle;
  std::vector<int32_t> m_clause_buf;

  std::function<bool()> m_terminate_callback;
};


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
