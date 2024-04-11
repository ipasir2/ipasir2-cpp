// C++ wrapper for IPASIR-2 solvers (current development version)
//
// https://github.com/ipasir2/ipasir2-cpp
//
// Copyright (c) 2024 Felix Kutzner
// This file is subject to the MIT license (https://spdx.org/licenses/MIT.html).
// SPDX-License-Identifier: MIT

/// \file
///
/// Functions for loading IPASIR-2 implementations at runtime. This header is
/// split off from ipasir2cpp.h to avoid including system headers there.

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
IPASIR2CPP_BEGIN_INLINE_NAMESPACE

namespace detail {
#if defined(WIN32)
  class dll_impl : public dll {
  public:
    explicit dll_impl(std::filesystem::path const& library)
      : m_lib_handle{LoadLibraryW(library.c_str())}, m_path{library}
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


/// \brief Creates an `ipasir2` object using an IPASIR-2 implementation selected at runtime.
///
/// \throws `ipasir2_error` if the library can't be loaded or IPASIR-2 symbols are missing
///         in the library.
inline ipasir2 create_api(std::filesystem::path const& library)
{
  return create_api(std::make_shared<detail::dll_impl>(library));
}

IPASIR2CPP_END_INLINE_NAMESPACE
}
