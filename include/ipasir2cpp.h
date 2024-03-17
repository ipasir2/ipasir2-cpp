#pragma once

#if __cplusplus < 201703L
#error "ipasir2cpp.h requires C++17 or newer"
#endif

#include <ipasir2.h>

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>


namespace ipasir2 {

class ipasir2_error : public std::runtime_error {
public:
  explicit ipasir2_error(ipasir2_errorcode) : runtime_error("IPASIR2 call failed"){};
};


namespace detail {
  inline void throw_if_failed(ipasir2_errorcode errorcode)
  {
    if (errorcode != IPASIR2_E_OK) {
      throw ipasir2_error{errorcode};
    }
  }


#if defined(__cpp_lib_concepts)
  template <typename T>
  constexpr bool is_contiguous_lit_iter = std::contiguous_iterator<T>;
#else
  template <typename T>
  constexpr bool is_contiguous_lit_iter
      = std::is_pointer_v<std::decay_t<T>>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::iterator>
        || std::is_same_v<std::decay_t<T>, std::vector<int32_t>::const_iterator>;
#endif
}


class solver {
public:
  solver() { detail::throw_if_failed(ipasir2_init(&m_handle)); }


  template <typename Iter>
  void add_clause(Iter start, Iter stop, ipasir2_redundancy redundancy = IPASIR2_R_NONE)
  {
    if constexpr (detail::is_contiguous_lit_iter<Iter>) {
      static_assert(std::is_same_v<std::decay_t<decltype(*start)>, int32_t>);

      int32_t const* buf = &*start;
      int32_t len = std::distance(start, stop);
      detail::throw_if_failed(ipasir2_add(m_handle, buf, len, redundancy));
    }
    else {
      static_assert(
          std::is_same_v<std::decay_t<typename std::iterator_traits<Iter>::value_type>, int32_t>);

      m_clause_buf.assign(start, stop);
      detail::throw_if_failed(
          ipasir2_add(m_handle, m_clause_buf.data(), m_clause_buf.size(), redundancy));
    }
  }


  template <typename Container>
  void add_clause(Container const& container, ipasir2_redundancy redundancy = IPASIR2_R_NONE)
  {
    add_clause(container.begin(), container.end(), redundancy);
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
