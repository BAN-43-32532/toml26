#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg["i"].as_or<std::int64_t>(0) == 42);
  static_assert(cfg["name"].as_or<std::int64_t>(-1) == -1);
  static_assert(cfg["missing"].as_or<std::int64_t>(-1) == -1);
  static_assert(cfg["tbl"]["x"].as_or<std::int64_t>(0) == 7);
  static_assert(cfg["tbl"]["missing"].as_or<std::int64_t>(9) == 9);
}
