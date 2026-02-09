#include "toml26/toml.hpp"

constexpr auto cfg = toml::parse<"a = 1 # bad \x01 comment\n">();

auto main() -> int {}
