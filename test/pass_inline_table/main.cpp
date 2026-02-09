#include <array>
#include <cstdint>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.site.nested.x == 7);
  static_assert(cfg.site.nested.y == 8);
  auto const x  = cfg["site"]["nested"]["x"].as<std::int64_t>();
  auto const y  = cfg["site", "nested", "y"].as<std::int64_t>();
  auto const ok = x == 7 && y == 8;
  if (!ok) {
    return 1;
  }
}
