#include <array>
#include <string_view>

#include "toml26/toml.hpp"
using namespace toml::literals;

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.get<"nodes"_k, 1_i, "name"_k>()} == "n1");
  static_assert(cfg.get<"nodes"_k, 0_i, "vals"_k, 1_i>() == 20);
  static_assert(cfg.get<"flag"_k>());

  auto const ok =
    std::string_view{cfg.get<"nodes"_k, 0_i, "name"_k>()} == "n0"
    && cfg.get<"nodes"_k, 1_i, "vals"_k, 0_i>() == 30
    && cfg.get<"flag"_k>();
  if (!ok) {
    return 1;
  }
}
