#include <ipasir2cpp.h>
#include <ipasir2cpp_dl.h>

#include "mock/ipasir2_mock.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <string_view>


namespace ip2 = ipasir2;

/// Class for managing the IPASIR2 mock library loaded at runtime
class mock_lib {
public:
  mock_lib() : m_dll{mock_dll_path}
  {
    m_dll.load_func_sym(m_new_fn, "new_ipasir2_mock");
    m_dll.load_func_sym(m_delete_fn, "delete_ipasir2_mock");
  }


  using new_fn = decltype(&new_ipasir2_mock);
  using delete_fn = decltype(&delete_ipasir2_mock);
  using mock_uptr = std::unique_ptr<ipasir2_mock, delete_fn>;


  mock_uptr create_ipasir2_mock()
  {
    auto result = mock_uptr{m_new_fn(), m_delete_fn};
    result->set_fail_observer([](std::string_view message) { FAIL(message); });
    return result;
  }


#if defined(WIN32)
  constexpr static std::string_view mock_dll_path = "./ipasir2mock.dll";
#else
  constexpr static std::string_view mock_dll_path = "./ipasir2mock.so";
#endif


private:
  new_fn m_new_fn = nullptr;
  delete_fn m_delete_fn = nullptr;
  ip2::detail::dll_impl m_dll;
};


TEST_CASE("Call functions in dynamically loaded IPASIR2 library")
{
  mock_lib mock_shared_obj;
  auto mock = mock_shared_obj.create_ipasir2_mock();

  ip2::ipasir2 api = ip2::create_api(mock_lib::mock_dll_path);

  std::vector<ipasir2_option> const test_options
      = {ipasir2_option{"test_option_1", -1000, 1000, IPASIR2_S_CONFIG, 1, 0, nullptr},
         ipasir2_option{"test_option_2", 0, 100, IPASIR2_S_SOLVING, 0, 1, nullptr},
         ipasir2_option{nullptr, 0, 0, IPASIR2_S_SOLVING, 0, 0, nullptr}};

  using opt_bool = ip2::optional_bool;

  mock->set_signature("test 1.0", IPASIR2_E_OK);
  CHECK(api.signature() == "test 1.0");

  {
    mock->expect_init_call(1);
    auto solver1 = api.create_solver();
    mock->expect_init_call(2);
    auto solver2 = api.create_solver();

    mock->set_options(1, test_options);
    mock->set_options(2, test_options);

    mock->expect_call(1, options_call{IPASIR2_E_OK});
    mock->expect_call(1, set_option_call{"test_option_1", 1, 0, IPASIR2_E_OK});

    mock->expect_call(1, add_call{{1, 2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{-1}, IPASIR2_R_NONE, IPASIR2_E_OK});

    mock->expect_call(1, set_export_call{true, 0, IPASIR2_E_OK});
    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});

    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
    mock->expect_call(1, val_call{2, 2, IPASIR2_E_OK});

    mock->expect_call(2, options_call{IPASIR2_E_OK});
    mock->expect_call(2, set_option_call{"test_option_2", 1, 0, IPASIR2_E_OK});
    mock->expect_call(2, add_call{{-1}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(2, solve_call{{1}, 20, IPASIR2_E_OK});
    mock->expect_call(2, failed_call{1, 1, IPASIR2_E_OK});


    solver1->set_option("test_option_1", 1);
    solver2->set_option("test_option_2", 1);

    solver1->add(1, 2);
    solver1->add(-1);
    solver2->add(-1);

    solver1->set_export_callback([](ip2::clause_view<int32_t>) {}, 0);
    solver1->set_terminate_callback([]() { return false; });

    CHECK(solver1->solve() == opt_bool{true});
    CHECK(solver1->lit_value(2) == opt_bool{true});

    CHECK(solver2->solve(std::array{1}) == opt_bool{false});
    CHECK(solver2->assumption_failed(1));
  }

  CHECK(!mock->has_outstanding_expects());
}
