#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.a} == "x#y");
  static_assert(std::string_view{cfg.b} == "p#q");
  static_assert(std::string_view{cfg.c} == "v");
  auto const ok = cfg["a"].asString() == "x#y" && cfg["b"].asString() == "p#q" && cfg["c"].asString() == "v";
  if (!ok) {
    return 1;
  }
}
