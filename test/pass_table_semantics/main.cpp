#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.x.y.z.w.v == 1);
  static_assert(std::string_view{cfg.x.name} == "rootx");
  static_assert(std::string_view{cfg.fruit.apple.color} == "red");
  static_assert(cfg.fruit.apple.taste.sweet);
  static_assert(cfg.fruit.apple.texture.smooth);
  auto const ok =
    cfg["x", "y", "z", "w", "v"].as<std::int64_t>() == 1 && cfg["fruit"]["apple"]["texture"]["smooth"].as<bool>();
  if (!ok) {
    return 1;
  }
}
