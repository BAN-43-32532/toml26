#include <array>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.d_leap.year == 2024 && cfg.d_leap.month == 2 && cfg.d_leap.day == 29);
  static_assert(cfg.d_century.year == 2000 && cfg.d_century.month == 2 && cfg.d_century.day == 29);
  static_assert(cfg.d_common.year == 1900 && cfg.d_common.month == 2 && cfg.d_common.day == 28);

  static_assert(cfg.dt_leap.date.year == 2024 && cfg.dt_leap.date.month == 2 && cfg.dt_leap.date.day == 29);
  static_assert(cfg.dt_leap.time.hour == 23 && cfg.dt_leap.time.minute == 59 && cfg.dt_leap.time.second == 59);
  static_assert(cfg.dt_leap.offsetMinutes == 0);

  static_assert(cfg.ldt_end_of_month.date.year == 2023);
  static_assert(cfg.ldt_end_of_month.date.month == 11);
  static_assert(cfg.ldt_end_of_month.date.day == 30);
  static_assert(cfg.ldt_end_of_month.time.hour == 14 && cfg.ldt_end_of_month.time.minute == 15);
  static_assert(!cfg.ldt_end_of_month.time.hasSecond);

  auto const d = cfg["d_leap"].as<toml::LocalDate>();
  if (!(d.year == 2024 && d.month == 2 && d.day == 29)) {
    return 1;
  }

  auto const odt = cfg["dt_leap"].as<toml::OffsetDateTime>();
  if (!(odt.date.year == 2024
        && odt.date.month == 2
        && odt.date.day == 29
        && odt.time.hour == 23
        && odt.time.minute == 59
        && odt.time.second == 59
        && odt.offsetMinutes == 0)) {
    return 2;
  }

  auto const ldt = cfg["ldt_end_of_month"].as<toml::LocalDateTime>();
  if (!(ldt.date.year == 2023
        && ldt.date.month == 11
        && ldt.date.day == 30
        && ldt.time.hour == 14
        && ldt.time.minute == 15)) {
    return 3;
  }
}
