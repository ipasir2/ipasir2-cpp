# ipasir2-cpp

C++ porcelain for IPASIR-2


**Caveat:** Like IPASIR-2, this project is work in progress.


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
  of a single header file `ipasir2cpp.h`, which only requires `ipasir2.h` in your include
  path. No compiler flags or `#define`s needed.

- **Flexible**. The wrapper supports solvers linked to your binary at build time as
  well as loading solver libraries at execution time. It is not necessary to link to
  an IPASIR-2 implementation at build time if you only want to load solvers at runtime.

- **Efficient**. `ipasir2-cpp` avoids adding overhead where possible.

- **Modern**. Features that are not part of C++17, but useful for an IPASIR-2 wrapper
  (concepts library, `std::span`) are used if supported by the standard library used by
  the client.


### Non-goals

- **Support for older C++ standards**. Supporting C++14 and earlier is out of scope for this
  project, since it would add significant complexity to the wrapper.

- **Support for IPASIR-1**. This is also not planned, to avoid extra complexity. The main IPASIR-2
  repository provides code for wrapping IPASIR-1 solvers in the IPASIR-2 API.

- **Out-of-the-box support for loading solvers at execution time on non-POSIX platforms**. For
  loading solvers at execution time, the wrapper only supports `dlopen()`. On other platforms,
  clients need to provide a `dlfcn.h` implementation, or to fill a structure containing the
  IPASIR-2 function pointers when instantiating the API.


## Supported compilers

`ipasir2cpp.h` and the tests require C++17. The code in the `examples` directory requires C++20.

**TODO:** minimum compiler versions



## Progress

All IPASIR-2 functions will eventually be supported. Currently, the wrapper covers the
following functions:

 - [x] `ipasir2_signature`
 - [x] `ipasir2_init`
 - [x] `ipasir2_release`
 - [x] `ipasir2_options`
 - [x] `ipasir2_set_option`
 - [x] `ipasir2_add`
 - [x] `ipasir2_solve`
 - [x] `ipasir2_val`
 - [x] `ipasir2_failed`
 - [x] `ipasir2_set_terminate`
 - [x] `ipasir2_set_export`
 - [ ] `ipasir2_set_import`
 - [ ] `ipasir2_set_notify`


Features:

 - [x] Support for `dlopen()`
 - [x] Support for custom clause types
 - [x] Support for custom literal types
 - [x] Support for functionality also covered by IPASIR-1
 - [x] Options API
 - [ ] Support for importing clauses
 - [ ] Support for observing the current partial assignment
 - [ ] Namespace versioning
