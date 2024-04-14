[![Builds and tests](https://github.com/ipasir2/ipasir2-cpp/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/ipasir2/ipasir2-cpp/actions/workflows/build-and-test.yml)

# ipasir2-cpp

C++ porcelain for IPASIR-2


**Caveat:** Like IPASIR-2, this project is work in progress. See [changelog](CHANGELOG.md).


## Integrating `ipasir2-cpp`

This library is header-only. The most convenient way of using this project is to add it as
a git submodule, and to add `include` to your include path. The headers depend on the main
IPASIR-2 header `ipasir2.h`, which is provided in `deps/ipasir2`.

In CMake projects, you can also add this project via `add_subdirectory()`, which creates a
target `ipasir2cpp` carrying all needed include paths (`include` and `deps/ipasir2`).

TODO: link latest release tag


## Example

See the `examples` directory. `01_readme.cpp` shows how to solve a simple problem instance:

```
namespace ip2 = ipasir2;
using namespace std::literals;

try {
  ip2::ipasir2 api = ip2::create_api();
  // For dynamic loading: ip2::ipasir2 api = ip2::create_api("./solver.so");

  std::unique_ptr<ip2::solver> solver = api.create_solver();
  solver->add(1, 2, 3);
  solver->add(-1);
  solver->add(-2);

  auto result = solver->solve();
  print("Result: {}", result.map("sat"sv, "unsat"sv, "unknown"sv));
}
catch (ip2::ipasir2_error const& error) {
  print("Could not solve the formula: {}", error.what());
}
```


## Design goals

- **No magic**. `ipasir2-cpp` is a thin, yet modern-C++ wrapper around the IPASIR-2
  API. It hides the C API, but offers a straightforward mental mapping to raw IPASIR-2.
  As an escape hatch, `ipasir2-cpp` still allows low-level access to solvers, so custom
  extensions of the IPASIR-2 API can be used together with the wrapper.

- **Easy to use**. `ipasir2-cpp` plays well with STL data structures, but also with
  custom clause and literal types. Integration is easy as well: `ipasir2-cpp` consists
  of two header files `ipasir2cpp.h` and (optionally) `ipasir2cpp_dl.h`, requiring only
  `ipasir2.h` in your include path. No compiler flags or `#define`s needed.

- **Flexible**. The wrapper supports solvers linked to your binary at build time as
  well as loading solver libraries at execution time. It is not necessary to link to
  an IPASIR-2 implementation at build time if you only want to load solvers at runtime.

- **Efficient**. `ipasir2-cpp` avoids adding overhead where possible.

- **Modern**. Post-C++17 features (for example improved detection of contiguous iterators
  via concepts, or `std::span` support) are used if available.


### Non-goals

- **Support for older C++ standards**. Supporting C++14 and earlier is out of scope for this
  project, since it would add significant complexity to the wrapper.

- **Support for IPASIR-1**. This is also not planned, to avoid extra complexity. The main IPASIR-2
  repository provides code for wrapping IPASIR-1 solvers in the IPASIR-2 API.


## Supported compilers

`ipasir2-cpp` requires a C++17-conformant compiler. The project is regularly tested with
recent versions of Clang, GCC and MSVC.

Building the tests and examples requires a C++20-conformant compiler.


## Supported platforms

`ipasir2cpp.h` is platform-independent. This header contains all functionality except
for loading solver libraries at runtime, which is implemented in a separate (and optional)
header `ipasir2cpp_dl.h` for POSIX and Windows systems.
