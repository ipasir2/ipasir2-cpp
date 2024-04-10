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


struct set_terminate_call {
  bool expect_nonnull_callback = true;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct set_export_call {
  bool expect_nonnull_callback = true;
  int32_t max_len = 0;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct options_call {
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


struct set_option_call {
  std::string name;
  int64_t value = 0;
  int64_t index = 0;
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

  using any_call = std::variant<add_call,
                                solve_call,
                                val_call,
                                failed_call,
                                set_terminate_call,
                                set_export_call,
                                options_call,
                                set_option_call>;

  virtual void expect_call(instance_id instance_id, any_call const& call) = 0;

  virtual void set_signature(std::string_view signature, ipasir2_errorcode result) = 0;

  virtual bool has_outstanding_expects() const = 0;

  virtual void simulate_terminate_callback_call(instance_id instance_id, int expected_cb_result)
      = 0;

  virtual void simulate_export_callback_call(instance_id instance_id,
                                             std::vector<int32_t> const& clause)
      = 0;

  virtual void set_options(instance_id instance_id, std::vector<ipasir2_option> const& options) = 0;

  virtual void* get_ipasir2_handle(instance_id instance_id) = 0;

  ipasir2_mock(ipasir2_mock const&) = delete;
  ipasir2_mock(ipasir2_mock&&) = delete;
  ipasir2_mock& operator=(ipasir2_mock const&) = delete;
  ipasir2_mock& operator=(ipasir2_mock&&) = delete;
};


extern "C" {
IPASIR_API ipasir2_mock* new_ipasir2_mock();
IPASIR_API void delete_ipasir2_mock(ipasir2_mock const* mock);
}
