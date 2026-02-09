#include "toml26/toml.hpp"

constexpr auto cfg = toml::parse<"a = 1\rb = 2\n">();

auto main() -> int {}
