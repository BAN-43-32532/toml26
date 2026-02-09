#include <array>
#include <cmath>
#include <cstdint>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.deci == 99);
  static_assert(cfg.zero == 0);
  static_assert(cfg.hexv == 0xDEADBEEFULL);
  static_assert(cfg.octv == 0755);
  static_assert(cfg.binv == 0b10100011);
  static_assert(cfg.fe == 1e6);
  static_assert(cfg.pi > 3.14 && cfg.pi < 3.15);
  static_assert(cfg.negf < 0.0);
  static_assert(cfg.pinf > 1e300);
  static_assert(cfg.nnan != cfg.nnan);
  auto const ok =
    cfg["deci"].as<std::int64_t>() == 99
    && cfg["hexv"].as<std::int64_t>() == 0xDEADBEEFULL
    && cfg["binv"].as<std::int64_t>() == 0b10100011
    && cfg["negf"].as<double>() < 0.0;
  if (!ok) {
    return 1;
  }
}
