#ifndef TOML26_JSON_EMIT_HPP
#define TOML26_JSON_EMIT_HPP

#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "json_types.hpp"

namespace toml::json_detail {
constexpr auto appendIndent(std::string& out, std::size_t depth, JsonFormat format) -> void {
  if (!format.pretty) {
    return;
  }
  auto const spaceCount = depth * static_cast<std::size_t>(format.indent);
  for (std::size_t i = 0; i < spaceCount; ++i) {
    out.push_back(' ');
  }
}

constexpr auto appendUint64(std::string& out, std::uint64_t value) -> void {
  if (value == 0) {
    out.push_back('0');
    return;
  }
  std::string reversed{};
  while (value > 0) {
    reversed.push_back(static_cast<char>('0' + (value % 10U)));
    value /= 10U;
  }
  for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
    out.push_back(*it);
  }
}

constexpr auto appendInt64(std::string& out, std::int64_t value) -> void {
  if (value < 0) {
    out.push_back('-');
    auto const magnitude = static_cast<std::uint64_t>(-(value + 1)) + 1U;
    appendUint64(out, magnitude);
    return;
  }
  appendUint64(out, static_cast<std::uint64_t>(value));
}

constexpr auto appendJsonEscapedString(std::string& out, std::string_view value) -> void {
  constexpr auto hex = std::string_view{"0123456789ABCDEF"};
  out.push_back('"');
  for (unsigned char c: value) {
    switch (c) {
    case '\"': out += "\\\""; break;
    case '\\': out += "\\\\"; break;
    case '\b': out += "\\b"; break;
    case '\f': out += "\\f"; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    default:
      if (c < 0x20U) {
        out += "\\u00";
        out.push_back(hex[(c >> 4) & 0x0F]);
        out.push_back(hex[c & 0x0F]);
      } else {
        out.push_back(static_cast<char>(c));
      }
      break;
    }
  }
  out.push_back('"');
}

constexpr auto appendTwoDigits(std::string& out, unsigned value) -> void {
  out.push_back(static_cast<char>('0' + (value / 10U) % 10U));
  out.push_back(static_cast<char>('0' + value % 10U));
}

constexpr auto appendDateText(std::string& out, LocalDate const& date) -> void {
  if (date.year >= 0 && date.year <= 9999) {
    auto const y = static_cast<unsigned>(date.year);
    out.push_back(static_cast<char>('0' + (y / 1000U) % 10U));
    out.push_back(static_cast<char>('0' + (y / 100U) % 10U));
    out.push_back(static_cast<char>('0' + (y / 10U) % 10U));
    out.push_back(static_cast<char>('0' + y % 10U));
  } else {
    appendInt64(out, static_cast<std::int64_t>(date.year));
  }
  out.push_back('-');
  appendTwoDigits(out, date.month);
  out.push_back('-');
  appendTwoDigits(out, date.day);
}

constexpr auto appendTimeText(std::string& out, LocalTime const& time) -> void {
  appendTwoDigits(out, time.hour);
  out.push_back(':');
  appendTwoDigits(out, time.minute);
  if (!time.hasSecond) {
    return;
  }
  out.push_back(':');
  appendTwoDigits(out, time.second);
  if (time.nanosecond == 0U) {
    return;
  }
  auto        rem      = time.nanosecond;
  std::string reversed{};
  while (rem > 0U) {
    reversed.push_back(static_cast<char>('0' + (rem % 10U)));
    rem /= 10U;
  }
  while (reversed.size() < 9U) {
    reversed.push_back('0');
  }
  std::string frac{};
  for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
    frac.push_back(*it);
  }
  while (!frac.empty() && frac.back() == '0') {
    frac.pop_back();
  }
  if (!frac.empty()) {
    out.push_back('.');
    out.append(frac);
  }
}

constexpr auto appendDouble(std::string& out, double value) -> void {
  constexpr auto max = std::numeric_limits<double>::max();
  if (value != value || value > max || value < -max) {
    out += "null";
    return;
  }
  if (value == 0.0) {
    out.push_back('0');
    return;
  }
  if (value < 0.0) {
    out.push_back('-');
    value = -value;
  }
  auto const asInt = static_cast<std::int64_t>(value);
  if (value <= static_cast<double>(std::numeric_limits<std::int64_t>::max()) && static_cast<double>(asInt) == value) {
    appendInt64(out, asInt);
    return;
  }

  int exp10 = 0;
  while (value >= 10.0) {
    value /= 10.0;
    ++exp10;
  }
  while (value < 1.0) {
    value *= 10.0;
    --exp10;
  }

  constexpr int                  precision = 15;
  std::vector<int>               digits(static_cast<std::size_t>(precision + 1), 0);
  for (int i = 0; i <= precision; ++i) {
    auto const d                        = static_cast<int>(value);
    digits[static_cast<std::size_t>(i)] = d;
    value                               = (value - static_cast<double>(d)) * 10.0;
    if (value < 0.0) {
      value = 0.0;
    }
  }

  if (digits[precision] >= 5) {
    for (int i = precision - 1; i >= 0; --i) {
      ++digits[static_cast<std::size_t>(i)];
      if (digits[static_cast<std::size_t>(i)] <= 9) {
        break;
      }
      digits[static_cast<std::size_t>(i)] = 0;
      if (i == 0) {
        digits[0] = 1;
        ++exp10;
      }
    }
  }

  out.push_back(static_cast<char>('0' + digits[0]));
  std::size_t last = precision - 1;
  while (last > 0 && digits[last] == 0) {
    --last;
  }
  if (last > 0) {
    out.push_back('.');
    for (std::size_t i = 1; i <= last; ++i) {
      out.push_back(static_cast<char>('0' + digits[i]));
    }
  }
  if (exp10 != 0) {
    out.push_back('e');
    appendInt64(out, exp10);
  }
}

constexpr auto appendJsonValue(std::string& out, ValueRef value, JsonFormat format, std::size_t depth) -> void;

struct ObjectEmitContext {
  std::string* out = nullptr;
  JsonFormat   format{};
  std::size_t  depth = 0;
  bool         first = true;
};

constexpr auto emitObjectEntry(void* context, std::string_view key, ValueRef const& value) -> void {
  auto& ctx = *static_cast<ObjectEmitContext*>(context);
  if (!ctx.first) {
    ctx.out->push_back(',');
  }
  if (ctx.format.pretty) {
    ctx.out->push_back('\n');
    appendIndent(*ctx.out, ctx.depth + 1, ctx.format);
  }
  appendJsonEscapedString(*ctx.out, key);
  if (ctx.format.pretty) {
    ctx.out->append(": ");
  } else {
    ctx.out->push_back(':');
  }
  appendJsonValue(*ctx.out, value, ctx.format, ctx.depth + 1);
  ctx.first = false;
}

constexpr auto appendJsonObject(std::string& out, ValueRef value, JsonFormat format, std::size_t depth) -> void {
  out.push_back('{');
  ObjectEmitContext ctx{&out, format, depth, true};
  if (value.forEachKeyValue != nullptr) {
    value.forEachKeyValue(value.ptr, &ctx, &emitObjectEntry);
  }
  if (format.pretty && !ctx.first) {
    out.push_back('\n');
    appendIndent(out, depth, format);
  }
  out.push_back('}');
}

constexpr auto appendJsonArray(std::string& out, ValueRef value, JsonFormat format, std::size_t depth) -> void {
  out.push_back('[');
  auto const count = (value.sizeOf != nullptr) ? value.sizeOf(value.ptr) : 0;
  for (std::size_t i = 0; i < count; ++i) {
    if (i != 0) {
      out.push_back(',');
    }
    if (format.pretty) {
      out.push_back('\n');
      appendIndent(out, depth + 1, format);
    }
    auto const element = value.lookupByIndex(value.ptr, i);
    appendJsonValue(out, element, format, depth + 1);
  }
  if (format.pretty && count > 0) {
    out.push_back('\n');
    appendIndent(out, depth, format);
  }
  out.push_back(']');
}

constexpr auto appendJsonValue(std::string& out, ValueRef value, JsonFormat format, std::size_t depth) -> void {
  switch (value.type) {
  case ValueType::string   : appendJsonEscapedString(out, value.asString()); break;
  case ValueType::integer  : appendInt64(out, value.as<std::int64_t>()); break;
  case ValueType::floating : appendDouble(out, value.as<double>()); break;
  case ValueType::boolean  : out += value.as<bool>() ? "true" : "false"; break;
  case ValueType::localDate: {
    auto text = std::string{};
    appendDateText(text, value.as<LocalDate>());
    appendJsonEscapedString(out, text);
    break;
  }
  case ValueType::localTime: {
    auto text = std::string{};
    appendTimeText(text, value.as<LocalTime>());
    appendJsonEscapedString(out, text);
    break;
  }
  case ValueType::localDateTime: {
    auto const ldt  = value.as<LocalDateTime>();
    auto       text = std::string{};
    appendDateText(text, ldt.date);
    text.push_back('T');
    appendTimeText(text, ldt.time);
    appendJsonEscapedString(out, text);
    break;
  }
  case ValueType::offsetDateTime: {
    auto const odt  = value.as<OffsetDateTime>();
    auto       text = std::string{};
    appendDateText(text, odt.date);
    text.push_back('T');
    appendTimeText(text, odt.time);
    auto const offset = odt.offsetMinutes;
    if (offset == 0) {
      text.push_back('Z');
    } else {
      auto const sign    = (offset < 0) ? '-' : '+';
      auto const abs     = static_cast<unsigned>(offset < 0 ? -offset : offset);
      auto const hours   = abs / 60U;
      auto const minutes = abs % 60U;
      text.push_back(sign);
      appendTwoDigits(text, hours);
      text.push_back(':');
      appendTwoDigits(text, minutes);
    }
    appendJsonEscapedString(out, text);
    break;
  }
  case ValueType::array: appendJsonArray(out, value, format, depth); break;
  case ValueType::table: appendJsonObject(out, value, format, depth); break;
  default              : out += "null"; break;
  }
}
}  // namespace toml::json_detail

#endif


