#include "example_utils.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {
void dump_file(fs::path const& path, std::string_view content)
{
  std::ofstream file{path};
  file.write(content.data(), content.size());
}


using formula = std::vector<std::vector<int32_t>>;


formula parse_cnf(std::string_view cnf)
{
  formula result;

  // TODO: delete file afterwards & use temporary directory
  std::string_view temp_file = "tmp.cnf";
  dump_file(temp_file, cnf);

  dimacs_parser parser{temp_file};
  parser.for_each_clause(
      [&](auto const& clause) { result.emplace_back(clause.begin(), clause.end()); });

  return result;
}
}


TEST_CASE("Read DIMACS file")
{
  auto input = R"(p cnf 4 2
1 2 0
-1 3 4 0
1 0
)";

  CHECK(parse_cnf(input) == formula{{1, 2}, {-1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS file without line breaks")
{
  auto input = R"(p cnf 4 2 1 2 0 -1 3 4 0 1 0)";
  CHECK(parse_cnf(input) == formula{{1, 2}, {-1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS empty file")
{
  auto input = R"(p cnf 0 0)";
  CHECK(parse_cnf(input) == formula{});
}


TEST_CASE("Read DIMACS empty file beginning with comment")
{
  auto input = R"(c comment comment2
p cnf 0 0)";
  CHECK(parse_cnf(input) == formula{});
}


TEST_CASE("Read DIMACS empty file beginning with empty comment")
{
  auto input = R"(c
p cnf 0 0)";
  CHECK(parse_cnf(input) == formula{});
}


TEST_CASE("Read DIMACS file with empty clause and no vars")
{
  auto input = R"(p cnf 0 1
                  0)";
  CHECK(parse_cnf(input) == formula{{}});
}


TEST_CASE("Read DIMACS file ending in comment")
{
  auto input = R"(p cnf 4 2
1 2 0
-1 3 4 0
1 0
c comment)";
  CHECK(parse_cnf(input) == formula{{1, 2}, {-1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS file with comments within clauses")
{
  auto input = R"(p cnf 4 2
1 2 0
c comment 1
-1 3 4 0

  c comment 2
1 0)";
  CHECK(parse_cnf(input) == formula{{1, 2}, {-1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS file ending in empty comment")
{
  auto input = R"(p cnf 4 2
1 2 0
-1 3 4 0
1 0
c)";
  CHECK(parse_cnf(input) == formula{{1, 2}, {-1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS file with empty clause and some vars")
{
  auto input = R"(p cnf 2 1
                  1 2 0
                  0
                  -1 -2 0)";
  CHECK(parse_cnf(input) == formula{{1, 2}, {}, {-1, -2}});
}


TEST_CASE("Read DIMACS file with comment starting in clause")
{
  auto input = R"(p cnf 4 2
1 2 c 1 4 5
-1 3 4 0
1 0)";
  CHECK(parse_cnf(input) == formula{{1, 2, -1, 3, 4}, {1}});
}


TEST_CASE("Read DIMACS file with out-of-range literal")
{
  CHECK_THROWS_AS(parse_cnf("p cnf 1 1\n-10000000000 0"), parse_error);
  CHECK_THROWS_AS(parse_cnf("p cnf 1 1\n10000000000 0"), parse_error);
}


TEST_CASE("Read DIMACS file with invalid literal")
{
  CHECK_THROWS_AS(parse_cnf("p cnf 1 1\n+10 0"), parse_error);
  CHECK_THROWS_AS(parse_cnf("p cnf 1 1\nabc 0"), parse_error);
  CHECK_THROWS_AS(parse_cnf("p cnf 1 1\n1abc 0"), parse_error);
}
