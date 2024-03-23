#include "example_utils.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <string>


namespace fs = std::filesystem;


stopwatch::stopwatch() : m_start{clock::now()} {}


std::chrono::milliseconds stopwatch::time_since_start() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_start);
}


// TODO: proper error handling for DIMACS parser

struct dimacs_token {
  std::optional<int32_t> literal;
  std::string_view string;

  size_t line = 0;
  size_t col = 0;
};


class bad_token_error : public std::exception {
public:
  char const* what() const noexcept override { return "bad token"; }
};


class dimacs_tokens {
public:
  explicit dimacs_tokens(fs::path const& path) : m_file{fopen(path.c_str(), "r"), fclose}
  {
    if (!m_file) {
      throw std::runtime_error{std::string{"Could not open file "} + path.string()};
    }
  }


  std::optional<dimacs_token> next_token()
  {
    while (true) {
      std::optional<char> char_past_word = read_word();

      if (m_string_buf.empty()) {
        break;
      }

      if (m_string_buf == "c") {
        if (char_past_word != '\n') {
          skip_to_next_line();
        }
        continue;
      }

      bool starts_like_int = m_string_buf[0] == '-' || isdigit(m_string_buf[0]);
      std::string_view rest = std::string_view(m_string_buf);

      if (starts_like_int && std::ranges::all_of(rest.substr(1), isdigit)) {
        try {
          return dimacs_token{std::stoi(m_string_buf), "", 0, 0};
        }
        catch (std::out_of_range const&) {
          throw bad_token_error{};
        }
      }

      return dimacs_token{std::nullopt, m_string_buf, 0, 0};
    }

    return std::nullopt;
  }


private:
  std::optional<char> read_char()
  {
    int result = fgetc(m_file.get());
    return result != EOF ? std::make_optional(static_cast<char>(result)) : std::nullopt;
  }


  std::optional<char> read_word()
  {
    m_string_buf.clear();
    std::optional<char> next = skip_whitespace();
    if (!next.has_value()) {
      return std::nullopt;
    }

    do {
      m_string_buf.push_back(*next);
      next = read_char();
    } while (next.has_value() && !isspace(*next));

    return next;
  }


  std::optional<char> skip_whitespace()
  {
    int c = ' ';
    while (isspace(c)) {
      c = fgetc(m_file.get());
      if (c == EOF) {
        return std::nullopt;
      }
    }

    return static_cast<char>(c);
  }


  void skip_to_next_line()
  {
    int c = ' ';
    while (c != '\n' && c != EOF) {
      c = fgetc(m_file.get());
    }
  }


  std::unique_ptr<FILE, decltype(&fclose)> m_file;
  std::string m_string_buf;
};


parse_error::parse_error(std::string_view message) : m_message{message} {}


char const* parse_error::what() const noexcept
{
  return m_message.c_str();
}


dimacs_parser::dimacs_parser(fs::path const& file) : m_tokens{std::make_unique<dimacs_tokens>(file)}
{
}


dimacs_parser::~dimacs_parser() = default;


void dimacs_parser::read_and_drop_string_token(std::string_view str)
{
  std::optional<dimacs_token> token = m_tokens->next_token();
  if (!token.has_value() || token->string != str) {
    throw parse_error{"invalid header: unexpected string"};
  }
}

void dimacs_parser::read_and_drop_int_token()
{
  std::optional<dimacs_token> token = m_tokens->next_token();
  if (!token.has_value() || !token->literal.has_value()) {
    throw parse_error{"invalid header: expected literal missing"};
  }
}


std::optional<int32_t> dimacs_parser::next_lit()
{
  try {
    if (!m_is_past_header) {
      read_and_drop_string_token("p");
      read_and_drop_string_token("cnf");
      read_and_drop_int_token();
      read_and_drop_int_token();
      m_is_past_header = true;
    }

    std::optional<dimacs_token> lit_token = m_tokens->next_token();

    if (!lit_token.has_value()) {
      return std::nullopt;
    }

    if (!lit_token->literal.has_value()) {
      throw parse_error{"invalid token"};
    }

    return lit_token->literal;
  }
  catch (bad_token_error const& error) {
    throw parse_error{error.what()};
  }
}


std::optional<std::span<int32_t const>> dimacs_parser::next_clause()
{
  m_clause_buf.clear();

  std::optional<int32_t> current_lit = next_lit();
  if (!current_lit.has_value()) {
    return std::nullopt;
  }

  while (current_lit.has_value() && current_lit != 0) {
    m_clause_buf.push_back(*current_lit);
    current_lit = next_lit();
  }

  if (!current_lit.has_value()) {
    throw parse_error{"unterminated clause"};
  }

  return m_clause_buf;
}


fs::path path_of_cnf(std::string_view name)
{
  return fs::path{__FILE__}.parent_path() / "input_files" / name;
}
