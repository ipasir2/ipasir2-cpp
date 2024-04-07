# `ipasir2-cpp` Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Wrapper functions covering the following IPASIR-2 functions:
  - `ipasir2_signature`
  - `ipasir2_init`
  - `ipasir2_release`
  - `ipasir2_options`
  - `ipasir2_set_option`
  - `ipasir2_add`
  - `ipasir2_solve`
  - `ipasir2_val`
  - `ipasir2_failed`
  - `ipasir2_set_terminate`
  - `ipasir2_set_export`
- Support for custom clause types
- Support for custom literal types
- Support for loading IPASIR-2 solvers at runtime
