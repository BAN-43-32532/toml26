#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.at_or<std::int64_t>("name", -1) == -1);
  static_assert(std::string_view{cfg.at_or<std::string_view>("i", std::string_view{"fallback"})} == "fallback");
  static_assert(!cfg.at_or("i", false));
  static_assert(cfg.at_or("ok", false));
}
