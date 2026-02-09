#ifndef TOML26_JSON_TYPES_HPP
#define TOML26_JSON_TYPES_HPP

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace toml {
[[noreturn]] constexpr auto failJson(std::string message) -> void { throw std::move(message); }

struct JsonFormat {
  bool     pretty = false;
  unsigned indent = 2;
};

template<std::size_t Capacity>
struct JsonBuffer {
  std::array<char, Capacity> bytes{};
  std::size_t                size = 0;

  constexpr auto push_back(char c) -> void {
    if (size >= Capacity) {
      failJson(std::string{"json buffer overflow"});
    }
    bytes[size++] = c;
  }

  constexpr auto append(std::string_view sv) -> void {
    for (char c: sv) {
      push_back(c);
    }
  }

  constexpr auto view() const -> std::string_view { return std::string_view{bytes.data(), size}; }
  constexpr      operator std::string_view() const { return view(); }
};
}  // namespace toml

#endif

