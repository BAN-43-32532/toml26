#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.arr.get<0>()} == "a,b");
  static_assert(std::string_view{cfg.arr.get<1>()} == "c#d");
  static_assert(std::string_view{cfg.arr.get<2>()} == "e");
  static_assert(std::string_view{cfg.tbl.x} == "1,2");
  static_assert(std::string_view{cfg.tbl.y} == "3#4");
  auto const ok =
    cfg["arr"][0].asString() == "a,b"
    && cfg["arr"][1].asString() == "c#d"
    && cfg["arr"][2].asString() == "e"
    && cfg["tbl"]["x"].asString() == "1,2"
    && cfg["tbl"]["y"].asString() == "3#4";
  if (!ok) {
    return 1;
  }
}
