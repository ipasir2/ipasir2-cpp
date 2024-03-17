#pragma once

#include <ipasir2.h>

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


class ipasir2 {
public:
  std::string signature() const
  {
    char const* result = nullptr;
    detail::throw_if_failed(ipasir2_signature(&result));
    return result;
  }
};

}
