#include <array>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.a.x == 1);
  static_assert(cfg.a.a == 2);
  auto const ok = cfg["a"]["x"].as<std::int64_t>() == 1 && cfg["a"]["a"].as<std::int64_t>() == 2;
  if (!ok) {
    return 1;
  }
}
