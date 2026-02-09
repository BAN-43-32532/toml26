#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.items.get<0>().name} == "a");
  static_assert(std::string_view{cfg.items.get<1>().name} == "b");
  auto const ok = cfg["items"][0]["name"].asString() == "a" && cfg["items"][1]["name"].asString() == "b";
  if (!ok) {
    return 1;
  }
}
