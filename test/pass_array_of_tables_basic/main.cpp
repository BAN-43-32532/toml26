#include <array>
#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.products.get<0>().name} == "Hammer");
  static_assert(cfg.products.get<0>().sku == 1);
  static_assert(std::string_view{cfg.products.get<1>().name} == "Nail");
  static_assert(cfg.products.get<1>().sku == 2);
  static_assert(std::string_view{cfg.products.get<1>().color} == "gray");
  static_assert(cfg.products.get<1>().dim.len == 3);
  if (cfg["products"][0]["name"].asString() != "Hammer") {
    return 11;
  }
  if (cfg["products"][0]["sku"].as<std::int64_t>() != 1) {
    return 12;
  }
  if (cfg["products"][1]["name"].asString() != "Nail") {
    return 13;
  }
  if (cfg["products"][1]["sku"].as<std::int64_t>() != 2) {
    return 14;
  }
  if (cfg["products"][1]["color"].asString() != "gray") {
    return 15;
  }
  if (cfg["products"][1]["dim"]["len"].as<std::int64_t>() != 3) {
    return 16;
  }
}
