#ifndef TOML26_JSON_HPP
#define TOML26_JSON_HPP

#include <string>
#include <string_view>

#include "json_emit.hpp"

namespace toml {
template<typename Root>
constexpr auto to_json(Root const& root, JsonFormat format = {}) -> std::string {
  auto out = std::string{};
  json_detail::appendJsonValue(out, ValueRef::from(root), format, 0);
  return out;
}

template<FixedString Source>
consteval auto to_json(JsonFormat format = {}) {
  auto const     json     = to_json(parse<Source>(), format);
  constexpr auto capacity = Source.view().size() * 8 + 256;
  auto           out      = JsonBuffer<capacity>{};
  out.append(std::string_view{json});
  return out;
}

template<auto SourceBytes>
consteval auto to_json(JsonFormat format = {}) {
  auto const     json     = to_json(parse<SourceBytes>(), format);
  constexpr auto capacity = SourceBytes.size() * 8 + 256;
  auto           out      = JsonBuffer<capacity>{};
  out.append(std::string_view{json});
  return out;
}
}  // namespace toml

#endif

