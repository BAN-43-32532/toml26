#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.arr.get<0>() == 1);
  static_assert(std::string_view{cfg.arr.get<1>().get<0>()} == "two");
  static_assert(cfg.arr.get<1>().get<1>() == 2);
  static_assert(cfg.arr.get<2>());
  static_assert(cfg.arr.get<3>().get<1>().get<0>() == 7);
  static_assert(std::string_view{cfg.arr_tables.get<0>().name} == "n0");
  static_assert(cfg.arr_tables.get<1>().score == 11);
  static_assert(cfg.mix.get<0>().a.get<1>().b == 2);
  auto const s0 = cfg["arr"][1][0].asString();
  auto const i0 = cfg["arr"][3][1][0].as<std::int64_t>();
  auto const i1 = cfg["arr", 3, 1, 0].as<std::int64_t>();
  auto const i2 = cfg["arr_tables"][1]["score"].as<std::int64_t>();
  auto const i3 = cfg["mix"][0]["a"][1]["b"].as<std::int64_t>();
  auto const ok = s0 == "two" && i0 == 7 && i1 == 7 && i2 == 11 && i3 == 2;
  if (!ok) {
    return 1;
  }
}
