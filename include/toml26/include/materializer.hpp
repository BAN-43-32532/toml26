#ifndef TOML26_MATERIALIZER_HPP
#define TOML26_MATERIALIZER_HPP

template<meta::info... Members>
struct RootAggregateDef {
  struct Aggregate;
  consteval { define_aggregate(^^Aggregate, {Members...}); }
};

template<meta::info... Members>
using GeneratedAggregate = RootAggregateDef<Members...>::Aggregate;

template<typename Rep>
struct ArrayObject: Rep {
  using TomlArrayTag = void;
  using StorageRep   = std::conditional_t<requires { typename Rep::UnderlyingRep; }, typename Rep::UnderlyingRep, Rep>;
  struct Iterator {
    ArrayObject const* owner = nullptr;
    std::size_t        index = 0;

    constexpr auto operator*() const -> ValueRef { return owner->indexLookup(index); }
    constexpr auto operator++() -> Iterator& {
      ++index;
      return *this;
    }
    constexpr friend auto operator==(Iterator const& lhs, Iterator const& rhs) -> bool {
      return lhs.owner == rhs.owner && lhs.index == rhs.index;
    }
  };

  constexpr ArrayObject(Rep value): Rep(value) {}

  static consteval auto staticSize() -> std::size_t {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^StorageRep, ctx));
    return members.size();
  }

  static consteval auto indices() { return std::define_static_array(std::views::iota(0zu, staticSize())); }

  constexpr auto size() const -> std::size_t { return staticSize(); }

  constexpr auto operator[](std::size_t idx) const -> ValueRef { return indexLookup(idx); }

  template<std::size_t I>
  constexpr decltype(auto) get() const {
    return getImpl<I, 0>();
  }

  template<typename First, typename... Rest>
  requires(path_detail::CtPathSegmentType<First> && (... && path_detail::CtPathSegmentType<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  template<auto First, auto... Rest>
  requires(path_detail::isCtPathSegmentValue<First> && (... && path_detail::isCtPathSegmentValue<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  constexpr auto indexLookup(std::size_t idx) const -> ValueRef {
    auto           out     = ValueRef{};
    std::size_t    i       = 0;
    constexpr auto ctx     = meta::access_context::current();
    auto const&    storage = static_cast<StorageRep const&>(*this);
    template for (constexpr auto m: define_static_array(nonstatic_data_members_of(^^StorageRep, ctx))) {
      if (!out.valid() && i == idx) {
        out = ValueRef::from(storage.[:m:]);
      }
      ++i;
    }
    return out;
  }

  constexpr auto begin() const -> Iterator { return Iterator{this, 0}; }
  constexpr auto end() const -> Iterator { return Iterator{this, size()}; }

 private:
  template<std::size_t I, std::size_t J>
  constexpr decltype(auto) getImpl() const {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^StorageRep, ctx));
    if constexpr (I >= members.size()) {
      static_assert(I < members.size(), "array index out of range");
    } else if constexpr (J >= members.size()) {
      static_assert(J < members.size(), "array index out of range");
    } else {
      constexpr auto m = members[J];
      if constexpr (I == J) {
        return (static_cast<StorageRep const&>(*this).[:m:]);
      } else {
        return getImpl<I, J + 1>();
      }
    }
  }
};

template<typename Rep, auto... Keys>
struct TableObject: Rep {
  using TomlTableTag  = void;
  using UnderlyingRep = Rep;
  struct Entry {
    std::string_view key{};
    ValueRef         value{};
  };
  struct Iterator {
    TableObject const* owner = nullptr;
    std::size_t        index = 0;

    constexpr auto operator*() const -> Entry { return Entry{owner->keyAt(index), owner->valueAt(index)}; }
    constexpr auto operator++() -> Iterator& {
      ++index;
      return *this;
    }
    constexpr friend auto operator==(Iterator const& lhs, Iterator const& rhs) -> bool {
      return lhs.owner == rhs.owner && lhs.index == rhs.index;
    }
  };

  static constexpr auto keyNames = std::array<std::string_view, sizeof...(Keys)>{std::string_view{Keys}...};

  constexpr explicit TableObject(Rep value): Rep(value) {}

  static consteval auto staticSize() -> std::size_t {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
    return members.size();
  }

  static consteval auto indices() { return std::define_static_array(std::views::iota(0zu, staticSize())); }

  template<std::size_t I>
  static consteval auto key() -> std::string_view {
    static_assert(I < keyNames.size(), "table index out of range");
    return keyNames[I];
  }

  constexpr auto lookupKey(std::string_view key) const -> ValueRef {
    auto           out = ValueRef{};
    std::size_t    i   = 0;
    constexpr auto ctx = meta::access_context::current();
    template for (constexpr auto m: define_static_array(nonstatic_data_members_of(^^Rep, ctx))) {
      if (!out.valid() && i < keyNames.size() && keyNames[i] == key) {
        out = ValueRef::from(this->[:m:]);
      }
      ++i;
    }
    return out;
  }

  constexpr auto operator[](std::string_view key) const -> ValueRef { return lookupKey(key); }

  template<path_detail::PathSegment... Rest>
  constexpr auto operator[](std::string_view key, Rest... rest) const -> ValueRef {
    auto next = (*this)[key];
    if constexpr (sizeof...(Rest) == 0) {
      return next;
    } else {
      return next[rest...];
    }
  }

  template<typename T>
  constexpr auto at(std::string_view key) const -> T {
    return (*this)[key].template as<T>();
  }

  template<typename T>
  constexpr auto at_or(std::string_view key, T defaultValue) const -> T {
    return (*this)[key].template as_or<T>(defaultValue);
  }

  template<typename T, path_detail::PathSegment First, path_detail::PathSegment... Rest>
  constexpr auto find(First first, Rest... rest) const -> std::optional<T> {
    return ValueRef::from(static_cast<Rep const&>(*this)).template find<T>(first, rest...);
  }

  template<std::size_t I>
  constexpr decltype(auto) get() const {
    return getByIndexImpl<I, 0>();
  }

  template<FixedString First, FixedString... Rest>
  constexpr decltype(auto) get() const {
    if constexpr (sizeof...(Rest) == 0) {
      return getImpl<First, 0>();
    } else {
      auto const& next = getImpl<First, 0>();
      if constexpr (requires { next.template get<Rest...>(); }) {
        return next.template get<Rest...>();
      } else {
        static_assert(requires { next.template get<Rest...>(); }, "compile-time path segment is not table-like");
      }
    }
  }

  template<typename First, typename... Rest>
  requires(path_detail::CtPathSegmentType<First> && (... && path_detail::CtPathSegmentType<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  template<auto First, auto... Rest>
  requires(path_detail::isCtPathSegmentValue<First> && (... && path_detail::isCtPathSegmentValue<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  constexpr auto begin() const -> Iterator { return Iterator{this, 0}; }
  constexpr auto end() const -> Iterator { return Iterator{this, keyNames.size()}; }

 private:
  constexpr auto keyAt(std::size_t idx) const -> std::string_view {
    if (idx >= keyNames.size()) {
      return {};
    }
    return keyNames[idx];
  }

  constexpr auto valueAt(std::size_t idx) const -> ValueRef {
    auto           out = ValueRef{};
    std::size_t    i   = 0;
    constexpr auto ctx = meta::access_context::current();
    template for (constexpr auto m: define_static_array(nonstatic_data_members_of(^^Rep, ctx))) {
      if (!out.valid() && i == idx) {
        out = ValueRef::from(this->[:m:]);
      }
      ++i;
    }
    return out;
  }

  template<std::size_t I, std::size_t J>
  constexpr decltype(auto) getByIndexImpl() const {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
    if constexpr (I >= members.size()) {
      static_assert(I < members.size(), "table index out of range");
    } else if constexpr (J >= members.size()) {
      static_assert(J < members.size(), "table index out of range");
    } else {
      constexpr auto m = members[J];
      if constexpr (I == J) {
        return (this->[:m:]);
      } else {
        return getByIndexImpl<I, J + 1>();
      }
    }
  }

  template<FixedString Key, std::size_t I>
  constexpr decltype(auto) getImpl() const {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
    if constexpr (I >= members.size()) {
      static_assert(I < members.size(), "compile-time key not found");
    } else {
      constexpr auto m = members[I];
      if constexpr (I < keyNames.size() && keyNames[I] == Key.view()) {
        return (this->[:m:]);
      } else {
        return getImpl<Key, I + 1>();
      }
    }
  }
};

template<typename Rep>
struct RootObject: Rep {
  constexpr explicit RootObject(Rep value): Rep(value) {}

  static consteval auto indices() {
    if constexpr (requires { Rep::indices(); }) {
      return Rep::indices();
    } else {
      constexpr auto ctx     = meta::access_context::current();
      constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
      return std::define_static_array(std::views::iota(0zu, members.size()));
    }
  }

  constexpr auto operator[](std::string_view key) const -> ValueRef {
    return ValueRef::from(static_cast<Rep const&>(*this))[key];
  }

  template<path_detail::PathSegment... Rest>
  constexpr auto operator[](std::string_view key, Rest... rest) const -> ValueRef {
    auto next = (*this)[key];
    if constexpr (sizeof...(Rest) == 0) {
      return next;
    } else {
      return next[rest...];
    }
  }

  template<typename T>
  constexpr auto at(std::string_view key) const -> T {
    return (*this)[key].template as<T>();
  }

  template<typename T>
  constexpr auto at_or(std::string_view key, T defaultValue) const -> T {
    return (*this)[key].template as_or<T>(defaultValue);
  }

  template<typename T, path_detail::PathSegment First, path_detail::PathSegment... Rest>
  constexpr auto find(First first, Rest... rest) const -> std::optional<T> {
    return ValueRef::from(static_cast<Rep const&>(*this)).template find<T>(first, rest...);
  }

  template<std::size_t I>
  constexpr decltype(auto) get() const {
    if constexpr (requires(Rep const& rep) { rep.template get<I>(); }) {
      return static_cast<Rep const&>(*this).template get<I>();
    } else {
      return getIndexImpl<I, 0>();
    }
  }

  template<FixedString First, FixedString... Rest>
  constexpr decltype(auto) get() const {
    if constexpr (sizeof...(Rest) == 0) {
      if constexpr (requires(Rep const& rep) { rep.template get<First>(); }) {
        return static_cast<Rep const&>(*this).template get<First>();
      } else {
        return getImpl<First, 0>();
      }
    } else {
      auto const& next = this->template get<First>();
      if constexpr (requires { next.template get<Rest...>(); }) {
        return next.template get<Rest...>();
      } else {
        static_assert(requires { next.template get<Rest...>(); }, "compile-time path segment is not table-like");
      }
    }
  }

  template<typename First, typename... Rest>
  requires(path_detail::CtPathSegmentType<First> && (... && path_detail::CtPathSegmentType<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  template<auto First, auto... Rest>
  requires(path_detail::isCtPathSegmentValue<First> && (... && path_detail::isCtPathSegmentValue<Rest>) )
  constexpr decltype(auto) get() const {
    return path_detail::ctPathGet<decltype(*this), First, Rest...>(*this);
  }

  constexpr auto begin() const {
    if constexpr (requires(Rep const& rep) { rep.begin(); }) {
      return static_cast<Rep const&>(*this).begin();
    } else {
      static_assert(requires(Rep const& rep) { rep.begin(); }, "root begin() is unavailable for this representation");
      return static_cast<Rep const&>(*this).begin();
    }
  }

  constexpr auto end() const {
    if constexpr (requires(Rep const& rep) { rep.end(); }) {
      return static_cast<Rep const&>(*this).end();
    } else {
      static_assert(requires(Rep const& rep) { rep.end(); }, "root end() is unavailable for this representation");
      return static_cast<Rep const&>(*this).end();
    }
  }

 private:
  template<std::size_t I, std::size_t J>
  constexpr decltype(auto) getIndexImpl() const {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
    if constexpr (I >= members.size()) {
      static_assert(I < members.size(), "compile-time index out of range");
    } else if constexpr (J >= members.size()) {
      static_assert(J < members.size(), "compile-time index out of range");
    } else {
      constexpr auto m = members[J];
      if constexpr (I == J) {
        return (this->[:m:]);
      } else {
        return getIndexImpl<I, J + 1>();
      }
    }
  }

  template<FixedString Key, std::size_t I>
  constexpr decltype(auto) getImpl() const {
    constexpr auto ctx     = meta::access_context::current();
    constexpr auto members = define_static_array(nonstatic_data_members_of(^^Rep, ctx));
    if constexpr (I >= members.size()) {
      static_assert(I < members.size(), "compile-time key not found");
    } else {
      constexpr auto m = members[I];
      if constexpr (identifier_of(m) == Key.view()) {
        return (this->[:m:]);
      } else {
        return getImpl<Key, I + 1>();
      }
    }
  }
};

template<typename Rep, auto... Values>
constexpr auto constructRoot = RootObject<Rep>{Rep{Values...}};

template<typename Rep, auto... Values>
constexpr auto constructArray = ArrayObject<Rep>{Rep{Values...}};

template<typename Rep, auto... Values>
constexpr auto constructFrom = Rep{Values...};

template<
  auto Path,
  auto type,
  auto InlineTable,
  auto ExplicitHeader,
  auto DottedDefined,
  auto ArrayContainer,
  auto LeadingOffset,
  auto LeadingCount,
  auto TrailingOffset,
  auto TrailingCount
>
constexpr auto constructMetaEntry = MetaEntry{
  Path,
  static_cast<ValueType>(type),
  InlineTable,
  ExplicitHeader,
  DottedDefined,
  ArrayContainer,
  static_cast<std::size_t>(LeadingOffset),
  static_cast<std::size_t>(LeadingCount),
  static_cast<std::size_t>(TrailingOffset),
  static_cast<std::size_t>(TrailingCount),
};

template<auto... Entries>
constexpr auto constructMetaArray = std::array{Entries...};

template<auto... Comments>
constexpr auto constructCommentArray = std::array{Comments...};

constexpr auto constructEmptyCommentArray = std::array<char const*, 0>{};

#endif

