#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.get<"sp ace">()} == "v1");
  static_assert(std::string_view{cfg.get<"lit.key">()} == "v2");
  static_assert(std::string_view{cfg.normal} == "v3");
  auto const ok =
    cfg["sp ace"].asString() == "v1" && cfg["lit.key"].asString() == "v2" && cfg["normal"].asString() == "v3";
  if (!ok) {
    return 1;
  }
}
