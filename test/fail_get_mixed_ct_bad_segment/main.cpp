#include <array>

#include "toml26/toml.hpp"
using namespace toml::literals;

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg = toml::parseEmbed<sourceBytes>();

auto main() -> int {
  static_assert(cfg.get<"nodes"_k, "bad"_k>() == 0);
}
