#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.items.get<0>().name} == "a");
  static_assert(cfg.items.get<0>().meta.v == 1);
  static_assert(std::string_view{cfg.items.get<1>().name} == "b");
  static_assert(cfg.items.get<1>().meta.v == 2);
  auto const ok =
    cfg["items"][0]["meta"]["v"].as<std::int64_t>() == 1 && cfg["items"][1]["meta"]["v"].as<std::int64_t>() == 2;
  if (!ok) {
    return 1;
  }
}
