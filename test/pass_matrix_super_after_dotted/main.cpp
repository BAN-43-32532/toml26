#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.a.b.c.v == 1);
  static_assert(std::string_view{cfg.a.name} == "ok");
  auto const ok = cfg["a"]["b"]["c"]["v"].as<std::int64_t>() == 1 && cfg["a"]["name"].asString() == "ok";
  if (!ok) {
    return 1;
  }
}
