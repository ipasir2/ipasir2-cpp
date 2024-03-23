#pragma once

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <vector>

// clang-format off
#define PRINT_FILENAME() do { std::cout << std::format("Running example: {}\n", __FILE__); } while(0)
// clang_format on


class stopwatch {
public:
  stopwatch();
  std::chrono::milliseconds time_since_start() const;

private:
  using clock = std::chrono::steady_clock;
  clock::time_point m_start;
};


std::filesystem::path path_of_cnf(std::string_view name);


template<typename... Ts>
void print(std::format_string<Ts...> format_string, Ts... args) {
  std::cout << std::format(format_string, std::forward<Ts>(args)...);
}


class dimacs_tokens;

class parse_error : public std::exception {
public:
  explicit parse_error(std::string_view message);
  char const* what() const noexcept override;

private:
  std::string m_message;
};


class dimacs_parser
{
public:
  explicit dimacs_parser(std::filesystem::path const& file);
  ~dimacs_parser();


  std::optional<std::span<int32_t const>> next_clause();


  template<typename Func>
  void for_each_clause(Func&& func) {
    auto clause = next_clause();
    while (clause.has_value()) {
      func(*clause);
      clause = next_clause();
    }
  }


private:
  void read_and_drop_string_token(std::string_view str);
  void read_and_drop_int_token();

  std::optional<int32_t> next_lit();
  std::unique_ptr<dimacs_tokens> m_tokens;
  bool m_is_past_header = false;
  std::vector<int32_t> m_clause_buf;
};
