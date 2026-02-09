#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.u1} == "A");
  static_assert(std::string_view{cfg.u2} == "B");
  auto const a = cfg["u1"].asString();
  auto const b = cfg["u2"].asString();
  auto const g = cfg["u3"].asString();
  auto const ok =
    a == "A"
    && b == "B"
    && g.size() == 2
    && static_cast<unsigned char>(g[0]) == 0xCEU
    && static_cast<unsigned char>(g[1]) == 0xB1U;
  if (!ok) {
    return 1;
  }
}
