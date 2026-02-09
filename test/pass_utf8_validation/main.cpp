#include <string_view>

#include "toml26/toml.hpp"

constexpr auto cfg = toml::parseEmbed<
#embed "case.toml"
>();

auto main() -> int {
  static_assert(std::string_view{cfg.name} == "ok");
}
