#include <array>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(std::string_view{cfg.fruit.name} == "apple");
  static_assert(std::string_view{cfg.fruit.variety.get<0>().name} == "red delicious");
  static_assert(std::string_view{cfg.fruit.variety.get<1>().name} == "granny smith");
  auto const ok =
    cfg["fruit"]["name"].asString() == "apple"
    && cfg["fruit"]["variety"][0]["name"].asString() == "red delicious"
    && cfg["fruit"]["variety"][1]["name"].asString() == "granny smith";
  if (!ok) {
    return 1;
  }
}
