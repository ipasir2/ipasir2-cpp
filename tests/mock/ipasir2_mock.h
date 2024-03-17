#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

#include <ipasir2.h>


struct add_call {
  std::vector<int> clause;
  ipasir2_redundancy redundancy = IPASIR2_R_NONE;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct solve_call {
  std::vector<int> assumptions;
  int result = 0;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct val_call {
  int32_t lit = 0;
  int32_t result = 0;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct failed_call {
  int32_t lit = 0;
  int32_t result = 0;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


class ipasir2_mock_error : public std::logic_error {
public:
  explicit ipasir2_mock_error(std::string_view message);
};


using instance_id = intptr_t;


class ipasir2_mock {
public:
  ipasir2_mock() = default;
  virtual ~ipasir2_mock() = default;

  /// Callback that is called when the IPASIR2 functions are called in
  /// some unexpected fashion (wrong order, wrong arguments, missing calls)
  ///
  /// The callback is supposed to call doctest's FAIL(). This cannot be done
  /// inside the mock, since the mock is compiled as a shared library (this,
  /// again, is used for testing the load-at-runtime behavior of the IPASIR2
  /// wrapper) and that in turn causes obscure symbol lookup problems when
  /// loading the mock library via dlopen().
  virtual void set_fail_observer(std::function<void(std::string_view)> const& callback) = 0;

  virtual void expect_init_call(instance_id instance_id) = 0;
  virtual void expect_init_call_and_fail(ipasir2_errorcode result) = 0;

  using any_call = std::variant<add_call, solve_call, val_call, failed_call>;
  virtual void expect_call(instance_id instance_id, any_call const& call) = 0;

  virtual void set_signature(std::string_view signature, ipasir2_errorcode result) = 0;

  virtual bool has_outstanding_expects() const = 0;

  ipasir2_mock(ipasir2_mock const&) = delete;
  ipasir2_mock(ipasir2_mock&&) = delete;
  ipasir2_mock& operator=(ipasir2_mock const&) = delete;
  ipasir2_mock& operator=(ipasir2_mock&&) = delete;
};


extern "C" {
ipasir2_mock* new_ipasir2_mock();
void delete_ipasir2_mock(ipasir2_mock const* mock);
}
