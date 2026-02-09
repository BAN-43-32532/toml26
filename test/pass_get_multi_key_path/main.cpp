#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.get<"a", "b", "c">() == 7);
  static_assert(cfg.get<"site", "google.com">());
  static_assert(!cfg.get<"site", "example.com">());
  static_assert(std::string_view{cfg.get<"meta", "info", "name">()} == "toml26");

  auto const ok =
    cfg.get<"a", "b", "c">() == 7
    && cfg.get<"site", "google.com">()
    && !cfg.get<"site", "example.com">()
    && std::string_view{cfg.get<"meta", "info", "name">()} == "toml26";
  if (!ok) {
    return 1;
  }
}
