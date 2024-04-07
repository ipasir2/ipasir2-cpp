// C++ wrapper for IPASIR-2 solvers
//
// Copyright (c) 2024 Felix Kutzner
// This file is subject to the MIT license (https://spdx.org/licenses/MIT.html).
// SPDX-License-Identifier: MIT

/// \file

#pragma once

#include <ipasir2cpp.h>

#include <filesystem>
#include <memory>


#if defined(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif


namespace ipasir2 {

namespace detail {
#if defined(WIN32)
  class dll_impl : public dll {
  public:
    explicit dll_impl(std::filesystem::path const& library)
      : m_lib_handle{LoadLibrary(library.c_str())}, m_path{library}
    {
      if (!m_lib_handle) {
        throw ipasir2_error{std::string{"Could not open "} + library.string()};
      }
    }


    ~dll_impl()
    {
      if (m_lib_handle) {
        FreeLibrary(m_lib_handle);
      }
    }


  protected:
    void* get_sym(std::string_view name) const override
    {
      void* result = reinterpret_cast<void*>(GetProcAddress(m_lib_handle, name.data()));
      if (result == nullptr) {
        throw ipasir2_error{"Symbol " + std::string{name} + " not found in " + m_path.string()};
      }
      return result;
    }


  private:
    HMODULE m_lib_handle;
    std::filesystem::path m_path;
  };

#else

  class dll_impl : public dll {
  public:
    explicit dll_impl(std::filesystem::path const& library)
      : m_lib_handle{dlopen(library.c_str(), RTLD_NOW)}, m_path{library}
    {
      if (m_lib_handle == nullptr) {
        throw ipasir2_error{std::string{"Could not open "} + library.string()};
      }
    }


    ~dll_impl()
    {
      if (m_lib_handle != nullptr) {
        dlclose(m_lib_handle);
      }
    }


  protected:
    void* get_sym(std::string_view name) const override
    {
      void* result = dlsym(m_lib_handle, name.data());
      if (result == nullptr) {
        throw ipasir2_error{"Symbol " + std::string{name} + " not found in " + m_path.string()};
      }
      return result;
    }


  private:
    void* m_lib_handle;
    std::filesystem::path m_path;
  };
#endif
}


inline ipasir2 create_api(std::filesystem::path const& library)
{
  return create_api(std::make_shared<detail::dll_impl>(library));
}
}
