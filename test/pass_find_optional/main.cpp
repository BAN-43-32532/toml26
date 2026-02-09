#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.find<std::int64_t>("id").has_value());
  static_assert(cfg.find<std::int64_t>("id").value() == 42);
  static_assert(!cfg.find<std::int64_t>("missing").has_value());
  static_assert(!cfg.find<std::int64_t>("name").has_value());

  static_assert(cfg.find<std::string_view>("name").has_value());
  static_assert(cfg.find<std::string_view>("name").value() == "orange");

  static_assert(cfg.find<std::int64_t>("tbl", "x").value_or(-1) == 7);
  static_assert(!cfg.find<std::int64_t>("tbl", "missing").has_value());

  static_assert(cfg.find<std::string_view>("arr", 1).value_or("x") == "b");
  static_assert(!cfg.find<std::string_view>("arr", 5).has_value());
  static_assert(!cfg.find<std::string_view>("name", 0).has_value());
}
