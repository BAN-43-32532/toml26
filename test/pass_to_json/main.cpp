#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "toml26/toml.hpp"

static constexpr auto sourceBytes = std::to_array<char>({
#embed "case.toml"
});

constexpr auto cfg       = toml::parseEmbed<sourceBytes>();
constexpr auto jsonCt    = toml::to_json<sourceBytes>();
constexpr auto jsonCtPre = toml::to_json<sourceBytes>({.pretty = true, .indent = 2});

static_assert(std::string_view{jsonCt}.starts_with("{"));
static_assert(std::string_view{jsonCt}.find("\"name\":\"Orange\"") != std::string_view::npos);
static_assert(std::string_view{jsonCt}.find("\"arr\":[1,\"x\",false]") != std::string_view::npos);
static_assert(std::string_view{jsonCt}.find("\"day\":\"2010-02-03\"") != std::string_view::npos);
static_assert(std::string_view{jsonCt}.find("\"clock\":\"14:15\"") != std::string_view::npos);
static_assert(std::string_view{jsonCt}.find("\"stamp\":\"2010-02-03T14:15:16Z\"") != std::string_view::npos);
static_assert(std::string_view{jsonCtPre}.find("\n  \"name\"") != std::string_view::npos);

auto main() -> int {
  auto const jsonRt = toml::to_json(cfg);
  auto const ok     = !jsonRt.empty() && jsonRt.find("\"tbl\":{\"x\":7}") != std::string::npos;
  if (!ok) {
    return 1;
  }
}
