#pragma once

#include <ipasir2.h>

#include <memory>
#include <stdexcept>
#include <string>

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
}


class solver {
public:
  solver() { detail::throw_if_failed(ipasir2_init(&m_handle)); }


  ~solver() { ipasir2_release(m_handle); }


  solver(solver const&) = delete;
  solver& operator=(solver const&) = delete;
  solver(solver&&) = delete;
  solver& operator=(solver&&) = delete;

private:
  void* m_handle = nullptr;
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
