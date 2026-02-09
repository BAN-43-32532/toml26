#include <print>
#include <string_view>

#include "toml26/toml.hpp"

constexpr auto cfg = toml::parse<R"(
name = "Orange"
physical.color = "orange"
physical.shape = "round"
site."google.com" = true

contributors = [
  "Foo Bar <foo@example.com>",
  { name = "Baz Qux", email = "bazqux@example.com", url = "https://example.com/bazqux" }
]
)">();

static_assert(std::string_view{cfg.name} == "Orange");
static_assert(std::string_view{cfg.physical.color} == "orange");
static_assert(std::string_view{cfg.physical.shape} == "round");

auto main() -> int {
  std::println("name: {}", cfg.name);
  std::println("color: {}", cfg.physical.color);
  std::println("shape: {}", cfg.physical.shape);
  std::println("google.com: {}", cfg["site"]["google.com"].as<bool>());
  std::println("contributor[0]: {}", cfg["contributors"][0].asString());
  std::println("contributor[1].name: {}", cfg["contributors"][1]["name"].asString());
  std::println("contributor[1].email: {}", cfg["contributors"][1]["email"].asString());
  std::println("contributor[1].url: {}", cfg["contributors"][1]["url"].asString());
}
