#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.name} == "Orange");
  static_assert(cfg.i == 42);
  static_assert(cfg.b);
  auto const runtimeName = cfg["name"].asString();
  auto const runtimeI    = cfg["i"].as<std::int64_t>();
  auto const runtimeB    = cfg["b"].as<bool>();
  auto const ok          = runtimeName == "Orange" && runtimeI == 42 && runtimeB;
  if (!ok) {
    return 1;
  }
}
