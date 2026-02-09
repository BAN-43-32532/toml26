#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

constexpr auto ctArrayIntSum = []() consteval {
  std::int64_t sum = 0;
  template for (constexpr auto i: decltype(cfg.arr)::indices()) {
    auto const& value = cfg.arr.get<i>();
    if constexpr (std::same_as<std::remove_cvref_t<decltype(value)>, std::int64_t>) {
      sum += value;
    }
  }
  return sum;
}();

constexpr auto ctSiteGoogle = []() consteval {
  bool found = false;
  template for (constexpr auto i: decltype(cfg.site)::indices()) {
    constexpr auto key = decltype(cfg.site)::key<i>();
    if constexpr (key == std::string_view{"google"}) {
      static_assert(cfg.site.get<i>());
      found = true;
    }
  }
  return found;
}();

auto main() -> int {
  static_assert(ctArrayIntSum == 1);
  static_assert(ctSiteGoogle);
}
