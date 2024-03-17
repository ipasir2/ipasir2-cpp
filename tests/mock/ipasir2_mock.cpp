#include "ipasir2_mock.h"

#include <algorithm>
#include <optional>
#include <queue>
#include <string_view>
#include <unordered_map>

#include <doctest.h>


class ipasir2_mock_impl;
static ipasir2_mock_impl* s_current_mock = nullptr;


ipasir2_mock_error::ipasir2_mock_error(std::string_view message) : std::logic_error(message.data())
{
}


struct mock_solver_instance {
  bool is_initialized = false;
  bool is_released = false;
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
      throw ipasir2_mock_error{"ipasir2_mock_impl already exists"};
    }

    s_current_mock = this;
  }


  virtual ~ipasir2_mock_impl() { s_current_mock = nullptr; }


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


private:
  std::unordered_map<instance_id, mock_solver_instance> m_instances;

  std::optional<next_init_call> m_next_init_call;
  std::vector<ipasir2_mock_error> m_mock_errors;

  std::string m_signature;
  std::optional<ipasir2_errorcode> m_signature_result;
};


std::unique_ptr<ipasir2_mock> create_ipasir2_mock()
{
  return std::make_unique<ipasir2_mock_impl>();
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
  FAIL(message);
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
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
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
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
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
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
}


ipasir2_errorcode
ipasir2_add(void* solver, int32_t const* clause, int32_t len, ipasir2_redundancy redundancy)
{
  try {
    throw_if_current_mock_is_null();
    instance_id instance = reinterpret_cast<instance_id>(solver);
    add_call spec = s_current_mock->pop_current_expected_call<add_call>(instance);

    std::vector<int> actual_clause{clause, clause + len};
    if (spec.clause != actual_clause) {
      throw ipasir2_mock_error{"ipasir2_add(): unexpected clause"};
    }

    if (spec.redundancy != redundancy) {
      throw ipasir2_mock_error{"ipasir2_add(): unexpected redundancy"};
    }

    return spec.return_value;
  }
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
}


ipasir2_errorcode ipasir2_solve(void* solver, int* result, int32_t const* assumptions, int32_t len)
{
  try {
    throw_if_current_mock_is_null();
    instance_id instance = reinterpret_cast<instance_id>(solver);
    solve_call spec = s_current_mock->pop_current_expected_call<solve_call>(instance);

    std::vector<int> actual_assumptions{assumptions, assumptions + len};
    if (spec.assumptions != actual_assumptions) {
      throw ipasir2_mock_error{"ipasir2_solve(): unexpected assumptions"};
    }

    *result = spec.result;
    return spec.return_value;
  }
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
}


ipasir2_errorcode ipasir2_val(void* solver, int32_t lit, int32_t* result)
{
  try {
    throw_if_current_mock_is_null();
    instance_id instance = reinterpret_cast<instance_id>(solver);
    val_call spec = s_current_mock->pop_current_expected_call<val_call>(instance);

    if (spec.lit != lit) {
      throw ipasir2_mock_error{"ipasir2_val(): unexpected literal"};
    }

    *result = spec.result;
    return spec.return_value;
  }
  catch (ipasir2_mock_error const& error) {
    fail_test(error.what());
    return IPASIR2_E_UNKNOWN;
  }
}
