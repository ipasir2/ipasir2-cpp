# ipasir2-cpp

C++ porcelain for IPASIR-2


**Caveat:** Like IPASIR-2, this project is work in progress. See [changelog](CHANGELOG.md).


## How to use

This library is header-only. An easy way for integrating this library is to add it as a Git
submodule, and add the `include` directory to your include path. Alternatively, you can copy
the headers into your application.

TODO: link latest release tag


## Example

See the `examples` directory. The most basic one is `01_readme.cpp`:

```
namespace ip2 = ipasir2;

try {
  ip2::ipasir2 api = ip2::create_api();
  // For dynamic loading: ip2::ipasir2 api = ip2::create_api("./solver.so");

  std::unique_ptr<ip2::solver> solver = api.create_solver();
  solver->add(1, 2, 3);
  solver->add(-1);
  solver->add(-2);

  if (auto result = solver->solve(); result.has_value()) {
    std::cout << std::format("Result: {}\n", result.unwrap() ? "SAT" : "UNSAT");
  }
  else {
    std::cout << "The solver did not produce a result.\n";
  }
}
catch (ip2::ipasir2_error const& error) {
  std::cerr << std::format("Could not solve the formula: {}\n", error.what());
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

`ipasir2cpp.h` and the tests require C++17. The code in the `examples` directory requires C++20.

**TODO:** minimum compiler versions


## Supported platforms

The platform-agnostic parts of `ipasir2-cpp` are defined in `ipasir2cpp.h`. This header contains
all functionality except for loading solver libraries at runtime, which is implemented in
a separate (and optional) header `ipasir2cpp_dl.h` for POSIX and Windows systems.
