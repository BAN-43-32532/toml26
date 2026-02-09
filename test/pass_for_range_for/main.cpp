#include <array>
#include <cstddef>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  std::size_t rootCount   = 0;
  std::size_t siteCount   = 0;
  std::size_t arrayCount  = 0;
  bool        sawSite     = false;
  bool        sawGoogle   = false;
  bool        sawSubArray = false;

  for (auto entry: cfg) {
    ++rootCount;
    if (entry.key == "site" && entry.value.type == toml::ValueType::table) {
      sawSite = true;
    }
  }
  for (auto entry: cfg.site) {
    ++siteCount;
    if (entry.key == "google" && entry.value.type == toml::ValueType::boolean && entry.value.as<bool>()) {
      sawGoogle = true;
    }
  }
  for (auto value: cfg.arr) {
    ++arrayCount;
    if (value.type == toml::ValueType::array) {
      sawSubArray = true;
    }
  }

  auto const ok = rootCount == 4 && siteCount == 3 && arrayCount == 4 && sawSite && sawGoogle && sawSubArray;
  if (!ok) {
    return 1;
  }
}
