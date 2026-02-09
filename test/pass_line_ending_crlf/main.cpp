#include <string_view>

#include "toml26/toml.hpp"

constexpr auto cfg = toml::parse<"a = 1\r\nb = true\r\n[tab]\r\nc = \"x\"\r\n">();

auto main() -> int {
  static_assert(cfg.a == 1);
  static_assert(cfg.b);
  static_assert(std::string_view{cfg.tab.c} == "x");
}
