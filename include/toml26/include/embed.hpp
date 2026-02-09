#ifndef TOML26_EMBED_HPP
#define TOML26_EMBED_HPP

#include <array>
#include <initializer_list>
#include <string>
#include <string_view>

namespace toml {
namespace detail {
template<int... Bytes>
consteval auto makeEmbeddedArray() -> std::array<char, sizeof...(Bytes)> {
  return std::array<char, sizeof...(Bytes)>{static_cast<char>(Bytes)...};
}
}  // namespace detail

constexpr auto normalizeEmbedded(std::string_view view) -> std::string_view {
  if (!view.empty() && view.back() == '\0') {
    view.remove_suffix(1);
  }
  return view;
}

template<auto SourceBytes>
consteval auto embed() -> std::string_view {
  return normalizeEmbedded(std::string_view{SourceBytes});
}

consteval auto embed(std::initializer_list<int> bytes) -> std::string {
  std::string out{};
  out.reserve(bytes.size());
  for (int const value: bytes) {
    if (value < 0 || value > 255) {
      throw std::string{"embed: byte out of range"};
    }
    out += static_cast<char>(value);
  }
  if (!out.empty() && out.back() == '\0') {
    out.pop_back();
  }
  return out;
}

template<auto SourceBytes>
consteval auto parseEmbed() {
  return parse<SourceBytes>();
}

template<int... Bytes>
consteval auto parseEmbed() {
  constexpr auto sourceBytes = detail::makeEmbeddedArray<Bytes...>();
  return parse<sourceBytes>();
}

template<int... Bytes>
consteval auto parseEmbedWithMeta() {
  constexpr auto sourceBytes = detail::makeEmbeddedArray<Bytes...>();
  return parse_with_meta<sourceBytes>();
}
}  // namespace toml

#endif

