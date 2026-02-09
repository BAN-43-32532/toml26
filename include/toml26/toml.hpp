#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "include/types.hpp"

namespace toml {
namespace meta = std::meta;

namespace path_detail {
template<typename T>
inline constexpr bool isKeyLike =
  std::same_as<std::remove_cvref_t<T>, std::string_view> || std::same_as<std::remove_cvref_t<T>, char const*>;

template<typename T>
inline constexpr bool isIndexLike =
  std::integral<std::remove_cvref_t<T>> && !std::same_as<std::remove_cvref_t<T>, bool>;

template<typename T>
concept PathSegment = isKeyLike<T> || isIndexLike<T>;
}  // namespace path_detail

[[noreturn]] constexpr auto fail(std::string message) -> void { throw std::move(message); }

struct ValueRef {
  using EmitKeyValueCallback = void (*)(void* context, std::string_view key, ValueRef const& value);
  using ForEachKeyValueFn    = void    (*)(void const* object, void* context, EmitKeyValueCallback emit);

  ValueType         type                                          = ValueType::none;
  void const*       ptr                                           = nullptr;
  ValueRef          (*lookupByKey)(void const*, std::string_view) = nullptr;
  ValueRef          (*lookupByIndex)(void const*, std::size_t)    = nullptr;
  std::size_t       (*sizeOf)(void const*)                        = nullptr;
  ForEachKeyValueFn forEachKeyValue                               = nullptr;

  constexpr auto valid() const -> bool { return type != ValueType::none; }

  constexpr auto operator[](std::string_view key) const -> ValueRef {
    if (type != ValueType::table || lookupByKey == nullptr) {
      fail(std::string{"invalid table key access"});
    }
    return lookupByKey(ptr, key);
  }

  constexpr auto operator[](std::size_t idx) const -> ValueRef {
    if (lookupByIndex == nullptr) {
      fail(std::string{"invalid array index access"});
    }
    return lookupByIndex(ptr, idx);
  }

  template<path_detail::PathSegment First, path_detail::PathSegment... Rest>
  requires(sizeof...(Rest) > 0)
  constexpr auto operator[](First first, Rest... rest) const -> ValueRef {
    auto next = step(first);
    return next[rest...];
  }

  template<typename T>
  constexpr auto at(std::string_view key) const -> T {
    return (*this)[key].as<T>();
  }

  template<typename T>
  constexpr auto at_or(std::string_view key, T defaultValue) const -> T {
    return (*this)[key].as_or<T>(defaultValue);
  }

  template<typename T>
  constexpr auto find() const -> std::optional<T> {
    if constexpr (std::same_as<T, std::string_view>) {
      if (type != ValueType::string) {
        return std::nullopt;
      }
      return asString();
    } else if constexpr (std::same_as<T, std::int64_t>) {
      if (type != ValueType::integer) {
        return std::nullopt;
      }
      return *static_cast<std::int64_t const*>(ptr);
    } else if constexpr (std::same_as<T, double>) {
      if (type != ValueType::floating) {
        return std::nullopt;
      }
      return *static_cast<double const*>(ptr);
    } else if constexpr (std::same_as<T, bool>) {
      if (type != ValueType::boolean) {
        return std::nullopt;
      }
      return *static_cast<bool const*>(ptr);
    } else if constexpr (std::same_as<T, OffsetDateTime>) {
      if (type != ValueType::offsetDateTime) {
        return std::nullopt;
      }
      return *static_cast<OffsetDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDateTime>) {
      if (type != ValueType::localDateTime) {
        return std::nullopt;
      }
      return *static_cast<LocalDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDate>) {
      if (type != ValueType::localDate) {
        return std::nullopt;
      }
      return *static_cast<LocalDate const*>(ptr);
    } else if constexpr (std::same_as<T, LocalTime>) {
      if (type != ValueType::localTime) {
        return std::nullopt;
      }
      return *static_cast<LocalTime const*>(ptr);
    } else {
      static_assert(!sizeof(T), "unsupported find<T>() type");
    }
  }

  template<typename T, path_detail::PathSegment First, path_detail::PathSegment... Rest>
  constexpr auto find(First first, Rest... rest) const -> std::optional<T> {
    auto next = stepMaybe(first);
    if (!next.has_value()) {
      return std::nullopt;
    }
    if constexpr (sizeof...(Rest) == 0) {
      return next->template find<T>();
    } else {
      return next->template find<T>(rest...);
    }
  }

  template<typename Visitor>
  constexpr decltype(auto) visit(Visitor&& visitor) const {
    switch (type) {
    case ValueType::string        : return std::forward<Visitor>(visitor)(*static_cast<char const* const*>(ptr));
    case ValueType::integer       : return std::forward<Visitor>(visitor)(*static_cast<std::int64_t const*>(ptr));
    case ValueType::floating      : return std::forward<Visitor>(visitor)(*static_cast<double const*>(ptr));
    case ValueType::boolean       : return std::forward<Visitor>(visitor)(*static_cast<bool const*>(ptr));
    case ValueType::offsetDateTime: return std::forward<Visitor>(visitor)(*static_cast<OffsetDateTime const*>(ptr));
    case ValueType::localDateTime : return std::forward<Visitor>(visitor)(*static_cast<LocalDateTime const*>(ptr));
    case ValueType::localDate     : return std::forward<Visitor>(visitor)(*static_cast<LocalDate const*>(ptr));
    case ValueType::localTime     : return std::forward<Visitor>(visitor)(*static_cast<LocalTime const*>(ptr));
    case ValueType::array         : return std::forward<Visitor>(visitor)(*this);
    case ValueType::table         : return std::forward<Visitor>(visitor)(*this);
    default                       : fail(std::string{"invalid value type"});
    }
  }

  constexpr auto asString() const -> std::string_view {
    if (type != ValueType::string) {
      fail(std::string{"type mismatch for asString"});
    }
    return std::string_view{*static_cast<char const* const*>(ptr)};
  }

  template<typename T>
  constexpr auto as() const -> T {
    if constexpr (std::same_as<T, std::string_view>) {
      return asString();
    } else if constexpr (std::same_as<T, std::int64_t>) {
      if (type != ValueType::integer) {
        fail(std::string{"type mismatch for as<int64_t>"});
      }
      return *static_cast<std::int64_t const*>(ptr);
    } else if constexpr (std::same_as<T, double>) {
      if (type != ValueType::floating) {
        fail(std::string{"type mismatch for as<double>"});
      }
      return *static_cast<double const*>(ptr);
    } else if constexpr (std::same_as<T, bool>) {
      if (type != ValueType::boolean) {
        fail(std::string{"type mismatch for as<bool>"});
      }
      return *static_cast<bool const*>(ptr);
    } else if constexpr (std::same_as<T, OffsetDateTime>) {
      if (type != ValueType::offsetDateTime) {
        fail(std::string{"type mismatch for as<OffsetDateTime>"});
      }
      return *static_cast<OffsetDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDateTime>) {
      if (type != ValueType::localDateTime) {
        fail(std::string{"type mismatch for as<LocalDateTime>"});
      }
      return *static_cast<LocalDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDate>) {
      if (type != ValueType::localDate) {
        fail(std::string{"type mismatch for as<LocalDate>"});
      }
      return *static_cast<LocalDate const*>(ptr);
    } else if constexpr (std::same_as<T, LocalTime>) {
      if (type != ValueType::localTime) {
        fail(std::string{"type mismatch for as<LocalTime>"});
      }
      return *static_cast<LocalTime const*>(ptr);
    } else {
      static_assert(!sizeof(T), "unsupported as<T>() type");
    }
  }

  template<typename T>
  constexpr auto as_or(T defaultValue) const -> T {
    if constexpr (std::same_as<T, std::string_view>) {
      if (type != ValueType::string) {
        return defaultValue;
      }
      return asString();
    } else if constexpr (std::same_as<T, std::int64_t>) {
      if (type != ValueType::integer) {
        return defaultValue;
      }
      return *static_cast<std::int64_t const*>(ptr);
    } else if constexpr (std::same_as<T, double>) {
      if (type != ValueType::floating) {
        return defaultValue;
      }
      return *static_cast<double const*>(ptr);
    } else if constexpr (std::same_as<T, bool>) {
      if (type != ValueType::boolean) {
        return defaultValue;
      }
      return *static_cast<bool const*>(ptr);
    } else if constexpr (std::same_as<T, OffsetDateTime>) {
      if (type != ValueType::offsetDateTime) {
        return defaultValue;
      }
      return *static_cast<OffsetDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDateTime>) {
      if (type != ValueType::localDateTime) {
        return defaultValue;
      }
      return *static_cast<LocalDateTime const*>(ptr);
    } else if constexpr (std::same_as<T, LocalDate>) {
      if (type != ValueType::localDate) {
        return defaultValue;
      }
      return *static_cast<LocalDate const*>(ptr);
    } else if constexpr (std::same_as<T, LocalTime>) {
      if (type != ValueType::localTime) {
        return defaultValue;
      }
      return *static_cast<LocalTime const*>(ptr);
    } else {
      static_assert(!sizeof(T), "unsupported as_or<T>() type");
    }
  }

 private:
  constexpr auto stepMaybe(std::string_view key) const -> std::optional<ValueRef> {
    if (type != ValueType::table || lookupByKey == nullptr) {
      return std::nullopt;
    }
    auto const next = lookupByKey(ptr, key);
    if (!next.valid()) {
      return std::nullopt;
    }
    return next;
  }

  constexpr auto stepMaybe(std::size_t idx) const -> std::optional<ValueRef> {
    if (lookupByIndex == nullptr) {
      return std::nullopt;
    }
    auto const next = lookupByIndex(ptr, idx);
    if (!next.valid()) {
      return std::nullopt;
    }
    return next;
  }

  template<path_detail::PathSegment Segment>
  constexpr auto stepMaybe(Segment segment) const -> std::optional<ValueRef> {
    if constexpr (path_detail::isKeyLike<Segment>) {
      return stepMaybe(std::string_view{segment});
    } else {
      using I = std::remove_cvref_t<Segment>;
      if constexpr (std::signed_integral<I>) {
        if (segment < 0) {
          return std::nullopt;
        }
      }
      return stepMaybe(static_cast<std::size_t>(segment));
    }
  }

  template<path_detail::PathSegment Segment>
  constexpr auto step(Segment segment) const -> ValueRef {
    if constexpr (path_detail::isKeyLike<Segment>) {
      return (*this)[std::string_view{segment}];
    } else {
      using I = std::remove_cvref_t<Segment>;
      if constexpr (std::signed_integral<I>) {
        if (segment < 0) {
          fail(std::string{"negative array index"});
        }
      }
      return (*this)[static_cast<std::size_t>(segment)];
    }
  }

  template<typename Table>
  static constexpr auto lookupTable(void const* p, std::string_view key) -> ValueRef {
    auto const&    obj = *static_cast<Table const*>(p);
    auto           out = ValueRef{};
    constexpr auto ctx = meta::access_context::current();
    template for (constexpr auto m: define_static_array(nonstatic_data_members_of(^^Table, ctx))) {
      if (!out.valid() && key == identifier_of(m)) {
        out = from(obj.[:m:]);
      }
    }
    return out;
  }

  template<typename Table>
  static constexpr auto lookupTableMapped(void const* p, std::string_view key) -> ValueRef {
    auto const& obj = *static_cast<Table const*>(p);
    return obj.lookupKey(key);
  }

  template<typename Table>
  static constexpr auto lookupTableIndex(void const* p, std::size_t idx) -> ValueRef {
    auto const& obj = *static_cast<Table const*>(p);
    auto        out = ValueRef{};
    if constexpr (requires { Table::indices(); }) {
      template for (constexpr auto i: Table::indices()) {
        if (!out.valid() && idx == i) {
          out = ValueRef::from(obj.template get<i>());
        }
      }
    }
    return out;
  }

  template<typename Array>
  static constexpr auto lookupArray(void const* p, std::size_t idx) -> ValueRef {
    auto const& obj = *static_cast<Array const*>(p);
    return obj.indexLookup(idx);
  }

  template<typename Array>
  static constexpr auto sizeOfArray(void const* p) -> std::size_t {
    auto const& obj = *static_cast<Array const*>(p);
    if constexpr (requires { obj.size(); }) {
      return obj.size();
    } else {
      return 0;
    }
  }

  template<typename Table>
  static constexpr auto forEachTableMapped(void const* p, void* context, EmitKeyValueCallback emit) -> void {
    auto const& obj = *static_cast<Table const*>(p);
    if constexpr (requires {
                    obj.begin();
                    obj.end();
                  }) {
      for (auto const entry: obj) {
        emit(context, entry.key, entry.value);
      }
    }
  }

  template<typename Table>
  static constexpr auto forEachTable(void const* p, void* context, EmitKeyValueCallback emit) -> void {
    auto const&    obj = *static_cast<Table const*>(p);
    constexpr auto ctx = meta::access_context::current();
    template for (constexpr auto m: define_static_array(nonstatic_data_members_of(^^Table, ctx))) {
      emit(context, identifier_of(m), from(obj.[:m:]));
    }
  }

 public:
  static consteval auto isPseudoArrayKey(std::string_view key, std::size_t idx) -> bool {
    if (key.size() < 3 || key[0] != 'm' || key[1] != '_') {
      return false;
    }
    auto const digits = key.substr(2);
    if (digits.empty()) {
      return false;
    }
    std::size_t value = 0;
    for (char const c: digits) {
      if (c < '0' || c > '9') {
        return false;
      }
      value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return value == idx;
  }

  template<typename Table>
  static consteval auto isPseudoArrayTable() -> bool {
    if constexpr (requires { Table::keyNames; }) {
      constexpr auto keys = Table::keyNames;
      if constexpr (keys.size() == 0) {
        return false;
      } else {
        for (std::size_t i = 0; i < keys.size(); ++i) {
          if (!isPseudoArrayKey(keys[i], i)) {
            return false;
          }
        }
        return true;
      }
    } else {
      return false;
    }
  }

  template<typename T>
  static constexpr auto from(T const& value) -> ValueRef {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::same_as<U, char const*>) {
      return ValueRef{ValueType::string, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, std::int64_t>) {
      return ValueRef{ValueType::integer, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, double>) {
      return ValueRef{ValueType::floating, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, bool>) {
      return ValueRef{ValueType::boolean, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, OffsetDateTime>) {
      return ValueRef{ValueType::offsetDateTime, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, LocalDateTime>) {
      return ValueRef{ValueType::localDateTime, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, LocalDate>) {
      return ValueRef{ValueType::localDate, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (std::same_as<U, LocalTime>) {
      return ValueRef{ValueType::localTime, std::addressof(value), nullptr, nullptr, nullptr, nullptr};
    } else if constexpr (requires { typename U::TomlArrayTag; }) {
      return ValueRef{ValueType::array, std::addressof(value), nullptr, &lookupArray<U>, &sizeOfArray<U>, nullptr};
    } else if constexpr (requires { typename U::TomlTableTag; }) {
      if constexpr (isPseudoArrayTable<U>()) {
        return ValueRef{
          ValueType::table,
          std::addressof(value),
          &lookupTableMapped<U>,
          &lookupTableIndex<U>,
          nullptr,
          &forEachTableMapped<U>
        };
      } else {
        return ValueRef{
          ValueType::table, std::addressof(value), &lookupTableMapped<U>, nullptr, nullptr, &forEachTableMapped<U>
        };
      }
    } else if constexpr (std::is_class_v<U>) {
      return ValueRef{ValueType::table, std::addressof(value), &lookupTable<U>, nullptr, nullptr, &forEachTable<U>};
    } else {
      static_assert(!sizeof(T), "unsupported reflected member type");
    }
  }
};

template<std::size_t N>
struct FixedString {
  std::array<char, N> chars{};

  consteval FixedString(char const (&s)[N]) { std::copy_n(s, N, chars.data()); }

  consteval auto view() const -> std::string_view { return std::string_view{chars.data(), N - 1}; }
};

template<std::size_t N>
FixedString(char const (&)[N]) -> FixedString<N>;

template<FixedString Key>
struct CtPathKey {
  static constexpr auto value = Key;
};

template<std::size_t Index>
struct CtPathIndex {
  static constexpr std::size_t value = Index;
};

namespace literals {
template<FixedString Key>
consteval auto operator""_k() -> CtPathKey<Key> {
  return {};
}

template<char C>
consteval auto parseIndexDigit() -> std::size_t {
  static_assert(C >= '0' && C <= '9', "index literal must be decimal digits");
  return static_cast<std::size_t>(C - '0');
}

template<std::size_t N>
struct Pow10 {
  static constexpr std::size_t value = 10zu * Pow10<N - 1>::value;
};

template<>
struct Pow10<0> {
  static constexpr std::size_t value = 1;
};

template<char... Chars>
struct ParseIndexLiteral;

template<char C>
struct ParseIndexLiteral<C> {
  static constexpr std::size_t value = parseIndexDigit<C>();
};

template<char C, char Next, char... Rest>
struct ParseIndexLiteral<C, Next, Rest...> {
  static constexpr std::size_t value =
    parseIndexDigit<C>() * Pow10<1 + sizeof...(Rest)>::value + ParseIndexLiteral<Next, Rest...>::value;
};

template<char... Chars>
inline constexpr std::size_t parseIndexLiteralV = ParseIndexLiteral<Chars...>::value;

template<char... Chars>
consteval auto operator""_i() -> CtPathIndex<parseIndexLiteralV<Chars...>> {
  static_assert(sizeof...(Chars) > 0, "empty index literal");
  return {};
}
}  // namespace literals

namespace path_detail {
template<typename T>
struct IsPathKeyType: std::false_type {};

template<FixedString Key>
struct IsPathKeyType<CtPathKey<Key>>: std::true_type {};

template<typename T>
struct IsPathIndexType: std::false_type {};

template<std::size_t Index>
struct IsPathIndexType<CtPathIndex<Index>>: std::true_type {};

template<typename T>
inline constexpr bool isPathKeyTypeV = IsPathKeyType<std::remove_cvref_t<T>>::value;

template<typename T>
inline constexpr bool isPathIndexTypeV = IsPathIndexType<std::remove_cvref_t<T>>::value;

template<typename T>
concept CtPathSegmentType = isPathKeyTypeV<T> || isPathIndexTypeV<T>;

template<auto Segment>
inline constexpr bool isCtPathSegmentValue = CtPathSegmentType<std::remove_cvref_t<decltype(Segment)>>;

template<typename Obj, auto Segment>
constexpr decltype(auto) ctPathStep(Obj const& obj) {
  using SegmentType = std::remove_cvref_t<decltype(Segment)>;
  if constexpr (isPathKeyTypeV<SegmentType>) {
    constexpr auto key = SegmentType::value;
    if constexpr (requires { obj.template get<key>(); }) {
      return obj.template get<key>();
    } else {
      static_assert(requires { obj.template get<key>(); }, "compile-time key segment cannot be applied here");
    }
  } else if constexpr (isPathIndexTypeV<SegmentType>) {
    constexpr auto index = SegmentType::value;
    if constexpr (requires { obj.template get<index>(); }) {
      return obj.template get<index>();
    } else {
      static_assert(requires { obj.template get<index>(); }, "compile-time index segment cannot be applied here");
    }
  } else {
    static_assert(CtPathSegmentType<SegmentType>, "unsupported compile-time path segment type");
  }
}

template<typename Obj, auto First, auto... Rest>
constexpr decltype(auto) ctPathGet(Obj const& obj) {
  decltype(auto) next = ctPathStep<Obj, First>(obj);
  if constexpr (sizeof...(Rest) == 0) {
    return next;
  } else {
    return ctPathGet<decltype(next), Rest...>(next);
  }
}
}  // namespace path_detail

#include "include/materializer.hpp"
#include "include/parser.hpp"

template<FixedString Source>
consteval auto parse() {
  constexpr auto err = detail::parseErrorOfText<Source>();
  if constexpr (err != detail::ParseError::none) {
    return detail::failParseValue<err>();
  } else {
    return [:parseAsReflection(Source.view()):];
  }
}

template<auto SourceBytes>
consteval auto parse() {
  constexpr auto err = detail::parseErrorOfBytes<SourceBytes>();
  if constexpr (err != detail::ParseError::none) {
    return detail::failParseValue<err>();
  } else {
    constexpr std::string_view sourceView{SourceBytes};
    return [:parseAsReflection(sourceView):];
  }
}

template<FixedString Source>
consteval auto parse_with_meta() {
  constexpr auto err = detail::parseErrorOfText<Source>();
  if constexpr (err != detail::ParseError::none) {
    return detail::failParseValue<err>();
  } else {
    auto data      = [:parseAsReflection(Source.view()):];
    auto entries   = [:parseMetaAsReflection(Source.view()):];
    auto comments  = [:parseCommentsAsReflection(Source.view()):];
    using MetaType = MetaRoot<std::tuple_size_v<decltype(entries)>, std::tuple_size_v<decltype(comments)>>;
    auto meta      = MetaType{entries, comments, {}};
    meta.global    = typename MetaType::Global{
      1,
      1,
      0,
      findRootMetaIndex(entries),
      std::tuple_size_v<decltype(entries)>,
      std::tuple_size_v<decltype(comments)>,
    };
    return ParseWithMetaOutput<decltype(data), MetaType>{data, meta};
  }
}

template<auto SourceBytes>
consteval auto parse_with_meta() {
  constexpr auto err = detail::parseErrorOfBytes<SourceBytes>();
  if constexpr (err != detail::ParseError::none) {
    return detail::failParseValue<err>();
  } else {
    constexpr std::string_view sourceView{SourceBytes};
    auto                       data     = [:parseAsReflection(sourceView):];
    auto                       entries  = [:parseMetaAsReflection(sourceView):];
    auto                       comments = [:parseCommentsAsReflection(sourceView):];
    using MetaType = MetaRoot<std::tuple_size_v<decltype(entries)>, std::tuple_size_v<decltype(comments)>>;
    auto meta      = MetaType{entries, comments, {}};
    meta.global    = typename MetaType::Global{
      1,
      1,
      0,
      findRootMetaIndex(entries),
      std::tuple_size_v<decltype(entries)>,
      std::tuple_size_v<decltype(comments)>,
    };
    return ParseWithMetaOutput<decltype(data), MetaType>{data, meta};
  }
}
}  // namespace toml

#include "include/embed.hpp"
#include "include/json.hpp"

