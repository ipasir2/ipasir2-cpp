#include "ipasir2_mock.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <string_view>
#include <unordered_map>


class ipasir2_mock_impl;
static ipasir2_mock_impl* s_current_mock = nullptr;


ipasir2_mock_error::ipasir2_mock_error(std::string_view message) : std::logic_error(message.data())
{
}


struct mock_solver_instance {
  bool is_initialized = false;
  bool is_released = false;
  std::vector<ipasir2_option> options;
  std::queue<ipasir2_mock::any_call> expected_calls;
};


struct next_init_call {
  std::optional<instance_id> id;
  ipasir2_errorcode return_value = IPASIR2_E_OK;
};


class ipasir2_mock_impl : public ipasir2_mock {
public:
  ipasir2_mock_impl()
  {
    if (s_current_mock != nullptr) {
      std::cerr << "Test setup failed: ipasir2_mock_impl already exists" << std::endl;
      std::terminate();
    }

    s_current_mock = this;
  }


  virtual ~ipasir2_mock_impl() { s_current_mock = nullptr; }


  void set_fail_observer(std::function<void(std::string_view)> const& callback) override
  {
    m_fail_observer = callback;
  }


  void expect_call(instance_id instance_id, any_call const& call) override
  {
    m_instances[instance_id].expected_calls.push(call);
  }


  void expect_init_call(instance_id instance_id) override
  {
    if (m_next_init_call.has_value()) {
      throw ipasir2_mock_error{"A new instance ID has been set by the test, but ipasir2_init() has "
                               "not been called since setting the previous ID"};
    }

    if (m_instances.find(instance_id) != m_instances.end()) {
      throw ipasir2_mock_error{"Test setup failed: the instance ID has already been used"};
    }

    m_next_init_call = next_init_call{instance_id, IPASIR2_E_OK};
  }


  void expect_init_call_and_fail(ipasir2_errorcode result) override
  {
    m_next_init_call = next_init_call{std::nullopt, result};
  }


  void set_signature(std::string_view signature, ipasir2_errorcode result) override
  {
    m_signature = signature;
    m_signature_result = result;
  }


  bool has_outstanding_expects() const override
  {
    if (m_next_init_call.has_value()) {
      return true;
    }

    return std::any_of(m_instances.begin(), m_instances.end(), [](auto const& instance) {
      return !instance.second.is_released;
    });
  }


  void simulate_terminate_callback_call(instance_id instance_id, int expected_cb_result) override
  {
    auto callback_iter = m_terminate_callbacks.find(instance_id);
    if (callback_iter == m_terminate_callbacks.end()) {
      throw ipasir2_mock_error{"Currently no terminate callback registered for the given instance"};
    }

    auto const& [callback, cookie] = callback_iter->second;

    int actual_cb_result = 0;
    try {
      actual_cb_result = callback(cookie);
    }
    catch (...) {
      fail_test("An exception was thrown from the terminate callback into the solver");
      return;
    }

    if (expected_cb_result != actual_cb_result) {
      throw ipasir2_mock_error{"Terminate callback returned unexpected result"};
    }
  }


  void simulate_export_callback_call(instance_id instance_id,
                                     std::vector<int32_t> const& clause) override
  {
    auto callback_iter = m_export_callbacks.find(instance_id);
    if (callback_iter == m_export_callbacks.end()) {
      throw ipasir2_mock_error{"Currently no export callback registered for the given instance"};
    }

    auto const& [callback, cookie] = callback_iter->second;

    try {
      callback(cookie, clause.data());
    }
    catch (...) {
      fail_test("An exception was thrown from export callback back into the solver");
    }
  }


  void set_options(instance_id instance_id, std::vector<ipasir2_option> const& options) override
  {
    m_instances[instance_id].options = options;
  }


  void* get_ipasir2_handle(instance_id instance_id) override
  {
    return reinterpret_cast<void*>(instance_id);
  }


  std::string const& get_signature() const { return m_signature; }


  ipasir2_errorcode get_signature_result() const
  {
    if (!m_signature_result.has_value()) {
      throw ipasir2_mock_error{"Unexpected call of ipasir2_signature()"};
    }

    return *m_signature_result;
  }


  template <typename CallT>
  CallT pop_current_expected_call(instance_id instance_id)
  {
    if (!is_alive(instance_id)) {
      throw ipasir2_mock_error{
          "IPASIR2 function called for released or not-yet-initialized solver object"};
    }

    auto& expected_calls = m_instances[instance_id].expected_calls;
    if (expected_calls.empty()) {
      throw ipasir2_mock_error{"IPASIR2 function called, but no further calls expected"};
    }

    try {
      CallT result = std::get<CallT>(expected_calls.front());
      expected_calls.pop();
      return result;
    }
    catch (std::bad_variant_access const&) {
      throw ipasir2_mock_error{"IPASIR2 function called, but different call expected"};
    }
  }


  next_init_call pop_next_instance_id()
  {
    if (!m_next_init_call.has_value()) {
      throw ipasir2_mock_error{"ipasir2_init() has been called unexpectedly"};
    }

    next_init_call result = *m_next_init_call;
    m_next_init_call.reset();
    return result;
  }


  void change_aliveness(instance_id instance_id, bool change_to_alive)
  {
    mock_solver_instance& instance = m_instances[instance_id];

    bool const to_alive_allowed = !instance.is_initialized && !instance.is_released;
    bool const to_dead_allowed = instance.is_initialized && !instance.is_released;

    if (change_to_alive && !to_alive_allowed) {
      throw ipasir2_mock_error{"IPASIR2 solver initialized twice, or initialized after release"};
    }

    if (!change_to_alive && !to_dead_allowed) {
      throw ipasir2_mock_error{"IPASIR2 solver released twice, or released before initialzed"};
    }

    if (change_to_alive) {
      instance.is_initialized = true;
    }
    else {
      instance.is_released = true;
    }
  }


  bool is_alive(instance_id instance_id) const
  {
    auto instance_iter = m_instances.find(instance_id);
    if (instance_iter == m_instances.end()) {
      throw ipasir2_mock_error{"IPASIR2 function called for unknown solver object"};
    }

    mock_solver_instance const& instance = instance_iter->second;
    return instance.is_initialized && !instance.is_released;
  }


  bool has_outstanding_expects(instance_id instance_id) const
  {
    auto instance_iter = m_instances.find(instance_id);
    if (instance_iter == m_instances.end()) {
      throw ipasir2_mock_error{"IPASIR2 function called for unknown solver object"};
    }

    return !instance_iter->second.expected_calls.empty();
  }


  void register_terminate_callback(instance_id solver,
                                   std::function<int(void*)> const& callback,
                                   void* cookie)
  {
    m_terminate_callbacks[solver] = {callback, cookie};
  }


  void clear_terminate_callback(instance_id solver) { m_terminate_callbacks.erase(solver); }


  void register_export_callback(instance_id solver,
                                std::function<void(void*, int32_t const*)> const& callback,
                                void* cookie)
  {
    m_export_callbacks[solver] = {callback, cookie};
  }


  void clear_export_callback(instance_id solver) { m_export_callbacks.erase(solver); }


  ipasir2_option const* get_options(instance_id solver) const
  {
    auto instance_iter = m_instances.find(solver);
    if (instance_iter == m_instances.end()) {
      throw ipasir2_mock_error{"IPASIR2 function called for unknown solver object"};
    }

    return instance_iter->second.options.data();
  }


  void fail_test(std::string_view message) { m_fail_observer(message); }


private:
  std::unordered_map<instance_id, mock_solver_instance> m_instances;

  std::optional<next_init_call> m_next_init_call;
  std::vector<ipasir2_mock_error> m_mock_errors;

  std::string m_signature;
  std::optional<ipasir2_errorcode> m_signature_result;

  std::function<void(std::string_view)> m_fail_observer;

  std::unordered_map<instance_id, std::pair<std::function<int(void*)>, void*>>
      m_terminate_callbacks;

  std::unordered_map<instance_id, std::pair<std::function<void(void*, int32_t const*)>, void*>>
      m_export_callbacks;
};


ipasir2_mock* new_ipasir2_mock()
{
  return new ipasir2_mock_impl();
}

void delete_ipasir2_mock(ipasir2_mock const* mock)
{
  delete mock;
}


namespace {
void throw_if_current_mock_is_null()
{
  if (s_current_mock == nullptr) {
    throw ipasir2_mock_error{"FATAL: test setup error: IPASIR2 mock function called, but no "
                             "ipasir2_mock instance exists"};
  }
}


void fail_test(std::string_view message)
{
  if (s_current_mock == nullptr) {
    std::cerr << message << std::endl;
    std::terminate();
  }
  else {
    s_current_mock->fail_test(message);
  }
}
}


ipasir2_errorcode ipasir2_signature(char const** result)
{
  try {
    throw_if_current_mock_is_null();
    std::string const& signature = s_current_mock->get_signature();
    *result = signature.c_str();
    return signature.empty() ? IPASIR2_E_UNSUPPORTED : IPASIR2_E_OK;
  }
  catch (std::exception const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
  catch (...) {
    fail_test("caught unknown exception in ipasir2_signature()");
    return IPASIR2_E_UNKNOWN;
  }
}


ipasir2_errorcode ipasir2_init(void** result)
{
  try {
    throw_if_current_mock_is_null();
    next_init_call next = s_current_mock->pop_next_instance_id();

    if (next.id.has_value()) {
      s_current_mock->change_aliveness(*next.id, true);
      *result = reinterpret_cast<void*>(*next.id);
    }

    return next.return_value;
  }
  catch (std::exception const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
  catch (...) {
    fail_test("caught unknown exception in ipasir2_init()");
    return IPASIR2_E_UNKNOWN;
  }
}


ipasir2_errorcode ipasir2_release(void* solver)
{
  try {
    throw_if_current_mock_is_null();
    instance_id instance = reinterpret_cast<instance_id>(solver);

    if (s_current_mock->has_outstanding_expects(instance)) {
      throw ipasir2_mock_error{
          "ipasir_release() has been called, but the instance has outstanding expected calls"};
    }

    s_current_mock->change_aliveness(instance, false);
    return IPASIR2_E_OK;
  }
  catch (std::exception const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
  catch (...) {
    fail_test("caught unknown exception in ipasir2_release()");
    return IPASIR2_E_UNKNOWN;
  }
}


namespace {
template <typename CallType>
ipasir2_errorcode
check_ipasir2_call(void* solver,
                   std::function<ipasir2_errorcode(CallType const& spec)> check_function)
{
  try {
    throw_if_current_mock_is_null();
    instance_id instance = reinterpret_cast<instance_id>(solver);
    CallType spec = s_current_mock->pop_current_expected_call<CallType>(instance);

    return check_function(spec);
  }
  catch (std::exception const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
  catch (...) {
    fail_test("caught unknown exception in IPASIR-2 function");
    return IPASIR2_E_UNKNOWN;
  }
}
}


ipasir2_errorcode
ipasir2_add(void* solver, int32_t const* clause, int32_t len, ipasir2_redundancy redundancy)
{
  return check_ipasir2_call<add_call>(solver, [&](add_call const& spec) {
    std::vector<int> actual_clause{clause, clause + len};
    if (spec.clause != actual_clause) {
      throw ipasir2_mock_error{"ipasir2_add(): unexpected clause"};
    }

    if (spec.redundancy != redundancy) {
      throw ipasir2_mock_error{"ipasir2_add(): unexpected redundancy"};
    }

    return spec.return_value;
  });
}


ipasir2_errorcode ipasir2_solve(void* solver, int* result, int32_t const* assumptions, int32_t len)
{
  return check_ipasir2_call<solve_call>(solver, [&](solve_call const& spec) {
    std::vector<int> actual_assumptions{assumptions, assumptions + len};
    if (spec.assumptions != actual_assumptions) {
      throw ipasir2_mock_error{"ipasir2_solve(): unexpected assumptions"};
    }

    *result = spec.result;
    return spec.return_value;
  });
}


ipasir2_errorcode ipasir2_val(void* solver, int32_t lit, int32_t* result)
{
  return check_ipasir2_call<val_call>(solver, [&](val_call const& spec) {
    if (spec.lit != lit) {
      throw ipasir2_mock_error{"ipasir2_val(): unexpected literal"};
    }

    *result = spec.result;
    return spec.return_value;
  });
}


ipasir2_errorcode ipasir2_failed(void* solver, int32_t lit, int32_t* result)
{
  return check_ipasir2_call<failed_call>(solver, [&](failed_call const& spec) {
    if (spec.lit != lit) {
      throw ipasir2_mock_error{"ipasir2_failed(): unexpected literal"};
    }

    *result = spec.result;
    return spec.return_value;
  });
}


ipasir2_errorcode ipasir2_set_terminate(void* solver, void* data, int (*callback)(void* data))
{
  return check_ipasir2_call<set_terminate_call>(solver, [&](set_terminate_call const& spec) {
    instance_id const solver_id = reinterpret_cast<instance_id>(solver);
    if (spec.expect_nonnull_callback) {
      if (data == nullptr || callback == nullptr) {
        throw ipasir2_mock_error{
            "ipasir2_set_terminate(): expected to get a callback, but it was cleared"};
      }

      s_current_mock->register_terminate_callback(solver_id, callback, data);
    }
    else {
      if (data != nullptr || callback != nullptr) {
        throw ipasir2_mock_error{
            "ipasir2_set_terminate(): expected the callback to be cleared, but it was set"};
      }

      if (spec.return_value == IPASIR2_E_OK) {
        s_current_mock->clear_terminate_callback(solver_id);
      }
    }

    return spec.return_value;
  });
}


ipasir2_errorcode
ipasir2_set_export(void* solver, void* data, int32_t max_len, void (*callback)(void*, int const*))
{
  return check_ipasir2_call<set_export_call>(solver, [&](set_export_call const& spec) {
    instance_id const solver_id = reinterpret_cast<instance_id>(solver);

    if (max_len != spec.max_len) {
      throw ipasir2_mock_error{"ipasir2_set_export(): unexpected max clause length"};
    }

    if (spec.expect_nonnull_callback) {
      if (data == nullptr || callback == nullptr) {
        throw ipasir2_mock_error{
            "ipasir2_set_export(): expected to get a callback, but it was cleared"};
      }

      s_current_mock->register_export_callback(solver_id, callback, data);
    }
    else {
      if (data != nullptr || callback != nullptr) {
        throw ipasir2_mock_error{
            "ipasir2_set_export(): expected the callback to be cleared, but it was set"};
      }

      if (spec.return_value == IPASIR2_E_OK) {
        s_current_mock->clear_terminate_callback(solver_id);
      }
    }

    return spec.return_value;
  });
}


ipasir2_errorcode ipasir2_options(void* solver, ipasir2_option const** options)
{
  return check_ipasir2_call<options_call>(solver, [&](options_call const& spec) {
    instance_id const solver_id = reinterpret_cast<instance_id>(solver);
    *options = s_current_mock->get_options(solver_id);
    return spec.return_value;
  });
}


ipasir2_errorcode
ipasir2_set_option(void* solver, ipasir2_option const* handle, int64_t value, int64_t index)
{
  return check_ipasir2_call<set_option_call>(solver, [&](set_option_call const& spec) {
    if (std::string_view{handle->name} != spec.name) {
      throw ipasir2_mock_error{"unexpected name"};
    }

    if (value != spec.value) {
      throw ipasir2_mock_error{"unexpected value"};
    }

    if (index != spec.index) {
      throw ipasir2_mock_error{"unexpected index"};
    }

    return spec.return_value;
  });
}
