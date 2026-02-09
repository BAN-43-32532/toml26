#include <cstdint>
#include <string_view>

#include "toml26/toml.hpp"

constexpr auto parsed = toml::parseEmbedWithMeta<
#embed "case.toml"
>();
auto const& [cfg, meta] = parsed;

constexpr auto hasMeta =
  [](auto const& entries, std::string_view path, toml::ValueType type, bool inlineTable) -> bool {
  for (auto const& entry: entries) {
    if (std::string_view{entry.path} == path && entry.type == type && entry.inlineTable == inlineTable) {
      return true;
    }
  }
  return false;
};

constexpr auto findMeta = [](auto const& entries, std::string_view path) -> toml::MetaEntry const* {
  for (auto const& entry: entries) {
    if (std::string_view{entry.path} == path) {
      return &entry;
    }
  }
  return nullptr;
};

constexpr auto hasCommentAt =
  [](auto const& pool, std::size_t offset, std::size_t count, std::string_view expected) -> bool {
  for (std::size_t i = 0; i < count; ++i) {
    if (std::string_view{pool[offset + i]} == expected) {
      return true;
    }
  }
  return false;
};

static_assert(std::string_view{cfg.name} == "Orange");
static_assert(std::string_view{cfg.physical.color} == "orange");
static_assert(cfg["nums"][1].as<std::int64_t>() == 2);
static_assert(hasMeta(meta.entries, "$", toml::ValueType::table, false));
static_assert(hasMeta(meta.entries, "name", toml::ValueType::string, false));
static_assert(hasMeta(meta.entries, "physical", toml::ValueType::table, false));
static_assert(hasMeta(meta.entries, "physical.color", toml::ValueType::string, false));
static_assert(hasMeta(meta.entries, "site", toml::ValueType::table, true));
static_assert(hasMeta(meta.entries, "nums", toml::ValueType::array, false));
static_assert(hasMeta(meta.entries, "contributors", toml::ValueType::array, false));
static_assert(hasMeta(meta.entries, "contributors[0]", toml::ValueType::table, false));
static_assert(hasMeta(meta.entries, "contributors[0].name", toml::ValueType::string, false));

static_assert(findMeta(meta.entries, "name") != nullptr);
static_assert(findMeta(meta.entries, "contributors[0]") != nullptr);
static_assert(findMeta(meta.entries, "$") != nullptr);
static_assert(hasCommentAt(
  meta.comments,
  findMeta(meta.entries, "name")->leadingOffset,
  findMeta(meta.entries, "name")->leadingCount,
  "name_doc"
));
static_assert(hasCommentAt(
  meta.comments,
  findMeta(meta.entries, "name")->trailingOffset,
  findMeta(meta.entries, "name")->trailingCount,
  "name_trail"
));
static_assert(hasCommentAt(
  meta.comments,
  findMeta(meta.entries, "contributors[0]")->leadingOffset,
  findMeta(meta.entries, "contributors[0]")->leadingCount,
  "contributors_doc"
));
static_assert(hasCommentAt(
  meta.comments,
  findMeta(meta.entries, "contributors[0]")->trailingOffset,
  findMeta(meta.entries, "contributors[0]")->trailingCount,
  "contributors_header_trail"
));
static_assert(hasCommentAt(
  meta.comments,
  findMeta(meta.entries, "$")->trailingOffset,
  findMeta(meta.entries, "$")->trailingCount,
  "eof_doc"
));

auto main() -> int {
  auto const ok =
    cfg["physical"]["shape"].asString() == std::string_view{"round"}
    && hasMeta(meta.entries, "site", toml::ValueType::table, true)
    && hasCommentAt(
      meta.comments,
      findMeta(meta.entries, "site")->trailingOffset,
      findMeta(meta.entries, "site")->trailingCount,
      "site_trail"
    )
    && hasCommentAt(
      meta.comments,
      findMeta(meta.entries, "contributors[0].name")->trailingOffset,
      findMeta(meta.entries, "contributors[0].name")->trailingCount,
      "contrib_name_trail"
    )
    && meta.global.rootIndex < meta.entries.size()
    && std::string_view{meta.entries[meta.global.rootIndex].path} == "$"
    && meta.global.tomlMajor == 1
    && meta.global.tomlMinor == 1
    && meta.global.tomlPatch == 0;
  if (!ok) {
    return 1;
  }
}
