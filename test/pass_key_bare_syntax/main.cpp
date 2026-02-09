#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.get<"bare-key">()} == "v1");
  static_assert(std::string_view{cfg.get<"1234">()} == "v2");
  static_assert(std::string_view{cfg.get<"mix_9-9">()} == "v3");
  static_assert(cfg.get<"table-1", "k-1">() == 7);
  static_assert(std::string_view{cfg.get<"123", "foo">()} == "bar");

  auto const v1 = cfg["bare-key"].asString();
  auto const v2 = cfg["1234"].asString();
  auto const v3 = cfg["table-1"]["k-1"].as<std::int64_t>();
  if (!(v1 == "v1" && v2 == "v2" && v3 == 7)) {
    return 1;
  }
}
