#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.tbl.a} == "k\n");
  static_assert(std::string_view{cfg.tbl.b} == "ok");
  auto const ok = cfg["tbl"]["a"].asString() == "k\n" && cfg["tbl"]["b"].asString() == "ok";
  if (!ok) {
    return 1;
  }
}
