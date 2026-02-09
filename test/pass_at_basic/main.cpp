#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.at<std::int64_t>("i") == 42);
  static_assert(cfg.at<bool>("ok"));
  static_assert(std::string_view{cfg.at<std::string_view>("name")} == "Orange");
  static_assert(cfg.tbl.at<std::int64_t>("x") == 7);
  static_assert(cfg["tbl"].at<std::int64_t>("x") == 7);
}
