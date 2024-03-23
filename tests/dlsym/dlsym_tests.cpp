#include <ipasir2cpp.h>

#include "mock/ipasir2_mock.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <dlfcn.h>

#include <array>
#include <filesystem>


namespace ip2 = ipasir2;

/// Class for managing the IPASIR2 mock library loaded at runtime
class mock_lib {
public:
  // The deleter type is spelled out explicitly, because gcc warns about ignored
  // attributes if decltype(&dlerror) is used instead
  using unique_lib_handle = std::unique_ptr<void, int (*)(void*)>;

  explicit mock_lib(std::filesystem::path const& path) : m_lib{nullptr, nullptr}
  {
    m_lib = unique_lib_handle{dlopen(path.c_str(), RTLD_NOW), dlclose};
    if (m_lib == nullptr) {
      throw std::runtime_error{dlerror()};
    }

    m_new_fn = reinterpret_cast<new_fn>(dlsym(m_lib.get(), "new_ipasir2_mock"));
    m_delete_fn = reinterpret_cast<delete_fn>(dlsym(m_lib.get(), "delete_ipasir2_mock"));

    if (m_new_fn == nullptr || m_delete_fn == nullptr) {
      throw std::runtime_error{dlerror()};
    }
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


private:
  new_fn m_new_fn = nullptr;
  delete_fn m_delete_fn = nullptr;
  unique_lib_handle m_lib;
};


TEST_CASE("Call functions in dynamically loaded IPASIR2 library")
{
  std::filesystem::path const library_file = "./libipasir2mock.so";
  mock_lib mock_shared_obj{library_file};
  auto mock = mock_shared_obj.create_ipasir2_mock();

  ip2::ipasir2 api = ip2::ipasir2::create(library_file);

  using opt_bool = ip2::optional_bool;

  mock->set_signature("test 1.0", IPASIR2_E_OK);
  CHECK_EQ(api.signature(), "test 1.0");

  {
    mock->expect_init_call(1);
    auto solver1 = api.create_solver();
    mock->expect_init_call(2);
    auto solver2 = api.create_solver();

    mock->expect_call(1, add_call{{1, 2}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(1, add_call{{-1}, IPASIR2_R_NONE, IPASIR2_E_OK});

    mock->expect_call(1, set_export_call{true, 0, IPASIR2_E_OK});
    mock->expect_call(1, set_terminate_call{true, IPASIR2_E_OK});

    mock->expect_call(1, solve_call{{}, 10, IPASIR2_E_OK});
    mock->expect_call(1, val_call{2, 2, IPASIR2_E_OK});

    mock->expect_call(2, add_call{{-1}, IPASIR2_R_NONE, IPASIR2_E_OK});
    mock->expect_call(2, solve_call{{1}, 20, IPASIR2_E_OK});
    mock->expect_call(2, failed_call{1, 1, IPASIR2_E_OK});

    solver1->add(1, 2);
    solver1->add(-1);
    solver2->add(-1);

    solver1->set_export_callback([](ip2::clause_view) {}, 0);
    solver1->set_terminate_callback([]() { return false; });

    CHECK_EQ(solver1->solve(), opt_bool{true});
    CHECK_EQ(solver1->lit_value(2), opt_bool{true});

    CHECK_EQ(solver2->solve(std::array{1}), opt_bool{false});
    CHECK(solver2->lit_failed(1));
  }

  CHECK(!mock->has_outstanding_expects());
}
