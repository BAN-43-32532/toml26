#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.name} == "ok");

  static_assert(std::string_view{cfg.m_00} == "v_m00");
  static_assert(std::string_view{cfg.m_01} == "v_m01");
  static_assert(std::string_view{cfg.m_awdwa} == "v_mawdwa");

  static_assert(std::string_view{cfg.get<"get">()} == "v_get");
  static_assert(std::string_view{cfg.get<"begin">()} == "v_begin");
  static_assert(std::string_view{cfg.get<"end">()} == "v_end");
  static_assert(std::string_view{cfg.get<"size">()} == "v_size");
  static_assert(std::string_view{cfg.get<"indices">()} == "v_indices");
  static_assert(std::string_view{cfg.get<"key">()} == "v_key");
  static_assert(std::string_view{cfg.get<"return">()} == "v_return");
  static_assert(std::string_view{cfg.get<"m_0">()} == "v_m0");
  static_assert(std::string_view{cfg.get<"m_1">()} == "v_m1");
  static_assert(std::string_view{cfg.get<"m_10">()} == "v_m10");
  static_assert(std::string_view{cfg.get<"_Xabc">()} == "v_upper");
  static_assert(std::string_view{cfg.get<"has__dunder">()} == "v_dunder");

  auto const ok =
    cfg["get"].asString() == "v_get"
    && cfg["begin"].asString() == "v_begin"
    && cfg["end"].asString() == "v_end"
    && cfg["size"].asString() == "v_size"
    && cfg["indices"].asString() == "v_indices"
    && cfg["key"].asString() == "v_key"
    && cfg["return"].asString() == "v_return"
    && cfg["m_0"].asString() == "v_m0"
    && cfg["m_1"].asString() == "v_m1"
    && cfg["m_10"].asString() == "v_m10"
    && cfg["_Xabc"].asString() == "v_upper"
    && cfg["has__dunder"].asString() == "v_dunder";

  if (!ok) {
    return 1;
  }
}
