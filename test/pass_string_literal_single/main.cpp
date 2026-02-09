#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.path} == "C:\\Users\\dev\\file.txt");
  static_assert(std::string_view{cfg.hash} == "#not_comment");
  static_assert(std::string_view{cfg.backslash} == "a\\b\\c");
  auto const ok =
    cfg["path"].asString() == "C:\\Users\\dev\\file.txt"
    && cfg["hash"].asString() == "#not_comment"
    && cfg["backslash"].asString() == "a\\b\\c";
  if (!ok) {
    return 1;
  }
}
