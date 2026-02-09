#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.a} == "line1\nline2");
  static_assert(std::string_view{cfg.b} == "tab\tend");
  static_assert(std::string_view{cfg.c} == "quote: \" and slash: \\");
  static_assert(std::string_view{cfg.d} == "hex:A");
  static_assert(std::string_view{cfg.e} == "esc:\x1B[");
  auto const ok =
    cfg["a"].asString() == "line1\nline2"
    && cfg["b"].asString() == "tab\tend"
    && cfg["c"].asString() == "quote: \" and slash: \\"
    && cfg["d"].asString() == "hex:A"
    && cfg["e"].asString() == "esc:\x1B[";
  if (!ok) {
    return 1;
  }
}
