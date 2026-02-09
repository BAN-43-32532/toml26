#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.physical.color} == "orange");
  static_assert(std::string_view{cfg.physical.shape} == "round");
  static_assert(cfg.site.nested.x == 7);
  auto const color = cfg["physical"]["color"].asString();
  auto const shape = cfg["physical", "shape"].asString();
  auto const x     = cfg["site", "nested", "x"].as<std::int64_t>();
  auto const ok    = color == "orange" && shape == "round" && x == 7;
  if (!ok) {
    return 1;
  }
}
