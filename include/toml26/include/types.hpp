#ifndef TOML26_TYPES_HPP
#define TOML26_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace toml {
struct LocalDate {
  int      year;
  unsigned month;
  unsigned day;
};

struct LocalTime {
  unsigned hour;
  unsigned minute;
  unsigned second;
  unsigned nanosecond;
  bool     hasSecond;
};

struct LocalDateTime {
  LocalDate date;
  LocalTime time;
};

struct OffsetDateTime {
  LocalDate date;
  LocalTime time;
  int       offsetMinutes;
};

template<typename Rep>
struct ArrayObject;

template<typename Rep, auto... Keys>
struct TableObject;

enum class ValueType : std::uint8_t {
  none,
  string,
  integer,
  floating,
  boolean,
  offsetDateTime,
  localDateTime,
  localDate,
  localTime,
  array,
  table,
};

struct MetaEntry {
  char const* path           = "";
  ValueType   type           = ValueType::none;
  bool        inlineTable    = false;
  bool        explicitHeader = false;
  bool        dottedDefined  = false;
  bool        arrayContainer = false;
  std::size_t leadingOffset  = 0;
  std::size_t leadingCount   = 0;
  std::size_t trailingOffset = 0;
  std::size_t trailingCount  = 0;
};

template<std::size_t EntryCount, std::size_t CommentCount>
struct MetaRoot {
  struct Global {
    std::uint16_t tomlMajor    = 1;
    std::uint16_t tomlMinor    = 1;
    std::uint16_t tomlPatch    = 0;
    std::size_t   rootIndex    = 0;
    std::size_t   entryCount   = EntryCount;
    std::size_t   commentCount = CommentCount;
  };

  std::array<MetaEntry, EntryCount>     entries{};
  std::array<char const*, CommentCount> comments{};
  Global                                global{};
};

template<typename Data, typename Meta>
struct ParseWithMetaOutput {
  Data data;
  Meta meta;
};
}  // namespace toml

#endif

