# ipasir2-cpp

C++ porcelain for IPASIR-2


**Caveat:** Like IPASIR-2, this project is work in progress.



## Design goals

- **No magic**. `ipasir2-cpp` is a thin, yet modern-C++ wrapper around the IPASIR-2 API. It hides the C API, but offers a straightforward mental mapping to raw IPASIR-2.

- **Easy to use**. `ipasir2-cpp` plays well with STL data structures, but also with custom clause and literal types. It
is more important to have an interface that is idiomatic and easy to use, than to have a simple wrapper implementation.

- **Flexible**. The wrapper supports solvers linked to your binary at build time as well as loading solver libraries at execution time.

- **Easy to integrate**. `ipasir2-cpp` consists of a single header file, `ipasir2cpp.h`, which only requires `ipasir2.h` in your include path. No `#define` needs to be set if you don't link an IPASIR-2 solver at build time.

- **Efficient**. `ipasir2-cpp` avoids adding overhead where possible.



## Progress

All IPASIR-2 functions will eventually be supported. Currently, the wrapper covers the
following functions:

 - [x] `ipasir2_signature`
 - [x] `ipasir2_init`
 - [x] `ipasir2_release`
 - [ ] `ipasir2_options`
 - [ ] `ipasir2_set_option`
 - [x] `ipasir2_add`
 - [x] `ipasir2_solve`
 - [x] `ipasir2_val`
 - [x] `ipasir2_failed`
 - [ ] `ipasir2_set_terminate`
 - [ ] `ipasir2_set_export`
 - [ ] `ipasir2_set_import`
 - [ ] `ipasir2_set_notify`



## Example

```
namespace ip2 = ipasir2;

try {
  ip2::ipasir2 api = ip2::ipasir2::create();
  // For dynamic loading: ip2::ipasir2 api = ip2::ipasir2::create("./solver.so");

  std::unique_ptr<ip2::solver> solver = api.create_solver();
  solver->add(1, 2, 3);
  solver->add(-1);
  solver->add(-2);

  if (auto result = solver->solve(); result.has_value()) {
    std::cout << "Result: " << (result.unwrap() ? "SAT" : "UNSAT") << "\n";
  }
  else {
    std::cout << "The solver did not produce a result.\n";
  }
}
catch(ip2::ipasir2_error const& error) {
  std::cerr << "Could not solve the formula: " << error.what() << "\n";
}
```

