#ifndef TOML26_PARSER_HPP
#define TOML26_PARSER_HPP

namespace detail {
enum class ParseError : std::uint8_t {
  none,
  malformedLine,
  invalidNewline,
  invalidComment,
  invalidUtf8,
  invalidKey,
  duplicateKey,
  invalidArray,
  invalidInlineTable,
  invalidString,
  invalidInteger,
  invalidFloat,
  invalidBoolean,
  invalidDate,
  invalidTime,
  invalidDateTime,
  unsupportedValue,
};

struct ParseOutput {
  struct MetaEntrySpec {
    std::string path{};
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

  std::vector<meta::info>               members{};
  std::vector<meta::info>               values{};
  std::vector<std::string>              keys{};
  std::vector<std::string>              memberNames{};
  std::vector<ValueType>                types{};
  std::vector<bool>                     inlineTables{};
  std::vector<std::vector<std::string>> leadingComments{};
  std::vector<std::vector<std::string>> trailingComments{};
  std::vector<MetaEntrySpec>            metaEntries{};
  std::vector<std::string>              comments{};
  std::size_t                           hiddenNameIndex = 0;
  ParseError                            error           = ParseError::none;
};

consteval auto makeParseOutput() -> ParseOutput {
  ParseOutput out{};
  out.values.emplace_back(^^void);
  return out;
}

struct NamedTableOutput {
  std::string                   name{};
  ParseOutput                   out{};
  std::vector<NamedTableOutput> children{};
  bool                          explicitHeader = false;
  bool                          dottedDefined  = false;
  bool                          arrayContainer = false;
  std::vector<std::string>      leadingComments{};
  std::vector<std::string>      trailingComments{};
  std::size_t                   nextArrayIndex = 0;
};

consteval auto startsWithPath(std::vector<std::string> const& whole, std::vector<std::string> const& prefix) -> bool {
  if (whole.size() < prefix.size()) {
    return false;
  }
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    if (whole[i] != prefix[i]) {
      return false;
    }
  }
  return true;
}

consteval auto containsKey(ParseOutput const& out, std::string_view key) -> bool {
  for (auto const& k: out.keys) {
    if (k == key) {
      return true;
    }
  }
  return false;
}

consteval auto findTableByName(std::vector<NamedTableOutput> const& tables, std::string_view key) -> std::size_t {
  for (std::size_t i = 0; i < tables.size(); ++i) {
    if (tables[i].name == key) {
      return i;
    }
  }
  return static_cast<std::size_t>(-1);
}

consteval auto
ensureChildTable(ParseOutput& parent, std::vector<NamedTableOutput>& children, std::string_view key, ParseError& error)
  -> NamedTableOutput* {
  if (containsKey(parent, key)) {
    error = ParseError::duplicateKey;
    return nullptr;
  }
  auto const idx = findTableByName(children, key);
  if (idx != static_cast<std::size_t>(-1)) {
    return std::addressof(children[idx]);
  }
  NamedTableOutput entry{};
  entry.name = std::string{key};
  entry.out  = makeParseOutput();
  children.emplace_back(entry);
  return std::addressof(children.back());
}

struct InlineParseResult {
  meta::info value = ^^void;
  ParseError error = ParseError::none;
};

struct ArrayParseResult {
  meta::info type  = ^^void;
  meta::info value = ^^void;
  ParseError error = ParseError::none;
};

template<class F>
consteval auto operator|(std::string_view sv, F&& parser) -> std::string_view {
  return std::forward<F>(parser)(sv);
}

constexpr auto isWs(char c) -> bool {
  return c == ' ' || c == '\t' || c == '\r';
}
constexpr auto isNl(char c) -> bool {
  return c == '\n';
}

constexpr auto isIdentStart(char c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr auto isIdentContinue(char c) -> bool {
  return isIdentStart(c) || (c >= '0' && c <= '9');
}

constexpr auto isBareKeyChar(char c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

consteval auto skipLine(std::string_view s, std::size_t pos) -> std::size_t {
  while (pos < s.size() && !isNl(s[pos])) {
    ++pos;
  }
  if (pos < s.size() && isNl(s[pos])) {
    ++pos;
  }
  return pos;
}

consteval auto skipInlineWs(std::string_view s, std::size_t pos) -> std::size_t {
  while (pos < s.size() && isWs(s[pos])) {
    ++pos;
  }
  return pos;
}

consteval auto trimRight(std::string_view s) -> std::string_view {
  auto end = s.size();
  while (end > 0 && isWs(s[end - 1])) {
    --end;
  }
  return s.substr(0, end);
}

consteval auto wsInline() {
  return [](std::string_view sv) consteval -> std::string_view { return sv.substr(skipInlineWs(sv, 0)); };
}

consteval auto wsLine() {
  return [](std::string_view sv) consteval -> std::string_view {
    auto i = std::size_t{0};
    while (i < sv.size() && (isWs(sv[i]) || isNl(sv[i]))) {
      ++i;
    }
    return sv.substr(i);
  };
}

consteval auto expectNonEmpty(ParseError code) {
  return [code](std::string_view sv) consteval -> std::string_view {
    if (sv.empty()) {
      throw code;
    }
    return sv;
  };
}

template<class Step>
consteval auto onParseError(ParseError& error, Step step) {
  return [&error, step](std::string_view sv) consteval -> std::string_view {
    try {
      return step(sv);
    } catch (ParseError code) {
      error = code;
      return {};
    }
  };
}

template<class Step>
consteval auto applyIf(bool const& condition, Step step) {
  return [&condition, step](std::string_view sv) consteval -> std::string_view {
    if (!condition) {
      return sv;
    }
    return step(sv);
  };
}

consteval auto consumeDot(bool& hasDot) {
  return [&hasDot](std::string_view sv) consteval -> std::string_view {
    hasDot = !sv.empty() && sv.front() == '.';
    if (!hasDot) {
      return sv;
    }
    return sv.substr(1);
  };
}

consteval auto makeArrayMemberName(std::size_t idx) -> std::string {
  std::string out = std::string{"m_"};
  if (idx == 0) {
    out.push_back('0');
    return out;
  }
  std::string reversed{};
  while (idx > 0) {
    reversed.push_back(static_cast<char>('0' + (idx % 10)));
    idx /= 10;
  }
  for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
    out.push_back(*it);
  }
  return out;
}

consteval auto parseUnsigned(std::string_view s, unsigned& out) -> bool {
  if (s.empty()) {
    return false;
  }
  unsigned value = 0;
  for (char c: s) {
    if (c < '0' || c > '9') {
      return false;
    }
    value = value * 10U + static_cast<unsigned>(c - '0');
  }
  out = value;
  return true;
}

constexpr auto isUtf8Cont(unsigned char c) -> bool {
  return c >= 0x80U && c <= 0xBFU;
}

constexpr auto isWellFormedUtf8(std::string_view s) -> bool {
  std::size_t i = 0;
  while (i < s.size()) {
    auto const b0 = static_cast<unsigned char>(s[i]);
    if (b0 <= 0x7FU) {
      ++i;
      continue;
    }
    if (b0 >= 0xC2U && b0 <= 0xDFU) {
      if (i + 1 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      if (!isUtf8Cont(b1)) {
        return false;
      }
      i += 2;
      continue;
    }
    if (b0 == 0xE0U) {
      if (i + 2 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      if (!(b1 >= 0xA0U && b1 <= 0xBFU) || !isUtf8Cont(b2)) {
        return false;
      }
      i += 3;
      continue;
    }
    if (b0 >= 0xE1U && b0 <= 0xECU) {
      if (i + 2 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      if (!isUtf8Cont(b1) || !isUtf8Cont(b2)) {
        return false;
      }
      i += 3;
      continue;
    }
    if (b0 == 0xEDU) {
      if (i + 2 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      if (!(b1 >= 0x80U && b1 <= 0x9FU) || !isUtf8Cont(b2)) {
        return false;
      }
      i += 3;
      continue;
    }
    if (b0 >= 0xEEU && b0 <= 0xEFU) {
      if (i + 2 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      if (!isUtf8Cont(b1) || !isUtf8Cont(b2)) {
        return false;
      }
      i += 3;
      continue;
    }
    if (b0 == 0xF0U) {
      if (i + 3 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      auto const b3 = static_cast<unsigned char>(s[i + 3]);
      if (!(b1 >= 0x90U && b1 <= 0xBFU) || !isUtf8Cont(b2) || !isUtf8Cont(b3)) {
        return false;
      }
      i += 4;
      continue;
    }
    if (b0 >= 0xF1U && b0 <= 0xF3U) {
      if (i + 3 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      auto const b3 = static_cast<unsigned char>(s[i + 3]);
      if (!isUtf8Cont(b1) || !isUtf8Cont(b2) || !isUtf8Cont(b3)) {
        return false;
      }
      i += 4;
      continue;
    }
    if (b0 == 0xF4U) {
      if (i + 3 >= s.size()) {
        return false;
      }
      auto const b1 = static_cast<unsigned char>(s[i + 1]);
      auto const b2 = static_cast<unsigned char>(s[i + 2]);
      auto const b3 = static_cast<unsigned char>(s[i + 3]);
      if (!(b1 >= 0x80U && b1 <= 0x8FU) || !isUtf8Cont(b2) || !isUtf8Cont(b3)) {
        return false;
      }
      i += 4;
      continue;
    }
    return false;
  }
  return true;
}

constexpr auto hasOnlyLfOrCrlf(std::string_view s) -> bool {
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\r') {
      if (i + 1 >= s.size() || s[i + 1] != '\n') {
        return false;
      }
      ++i;
    }
  }
  return true;
}

constexpr auto isDisallowedCommentControl(char c) -> bool {
  auto const uc = static_cast<unsigned char>(c);
  return uc <= 0x08U || (uc >= 0x0AU && uc <= 0x1FU) || uc == 0x7FU;
}

consteval auto digitValue(char c) -> int {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

consteval auto parseUnderscoredUnsigned(std::string_view s, unsigned base, unsigned long long& out, unsigned& digits)
  -> bool {
  if (s.empty()) {
    return false;
  }
  unsigned long long value          = 0;
  bool               prevUnderscore = false;
  digits                            = 0;
  for (char c: s) {
    if (c == '_') {
      if (digits == 0 || prevUnderscore) {
        return false;
      }
      prevUnderscore = true;
      continue;
    }
    auto const d = digitValue(c);
    if (d < 0 || static_cast<unsigned>(d) >= base) {
      return false;
    }
    if (value > (std::numeric_limits<unsigned long long>::max() - static_cast<unsigned long long>(d)) / base) {
      return false;
    }
    value          = value * base + static_cast<unsigned long long>(d);
    prevUnderscore = false;
    ++digits;
  }
  if (digits == 0 || prevUnderscore) {
    return false;
  }
  out = value;
  return true;
}

consteval auto parseSpecialFloat(std::string_view s, double& out) -> bool {
  if (s == "inf" || s == "+inf") {
    out = std::numeric_limits<double>::infinity();
    return true;
  }
  if (s == "-inf") {
    out = -std::numeric_limits<double>::infinity();
    return true;
  }
  if (s == "nan" || s == "+nan") {
    out = std::numeric_limits<double>::quiet_NaN();
    return true;
  }
  if (s == "-nan") {
    out = -std::numeric_limits<double>::quiet_NaN();
    return true;
  }
  return false;
}

consteval auto parseInt64(std::string_view s, std::int64_t& out) -> bool {
  if (s.empty()) {
    return false;
  }

  bool        neg = false;
  std::size_t pos = 0;
  if (s[pos] == '+' || s[pos] == '-') {
    neg = s[pos] == '-';
    ++pos;
  }
  if (pos >= s.size()) {
    return false;
  }

  unsigned base = 10;
  if (s.substr(pos).starts_with("0x") || s.substr(pos).starts_with("0X")) {
    if (neg || s.front() == '+') {
      return false;
    }
    base = 16;
    pos += 2;
  } else if (s.substr(pos).starts_with("0o") || s.substr(pos).starts_with("0O")) {
    if (neg || s.front() == '+') {
      return false;
    }
    base = 8;
    pos += 2;
  } else if (s.substr(pos).starts_with("0b") || s.substr(pos).starts_with("0B")) {
    if (neg || s.front() == '+') {
      return false;
    }
    base = 2;
    pos += 2;
  }

  auto const body = s.substr(pos);
  if (body.empty()) {
    return false;
  }

  unsigned long long mag    = 0;
  unsigned           digits = 0;
  if (!parseUnderscoredUnsigned(body, base, mag, digits)) {
    return false;
  }

  if (base == 10 && digits > 1 && body.front() == '0') {
    return false;
  }

  if (neg) {
    constexpr auto limit = static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max()) + 1ULL;
    if (mag > limit) {
      return false;
    }
    if (mag == limit) {
      out = std::numeric_limits<std::int64_t>::min();
    } else {
      out = -static_cast<std::int64_t>(mag);
    }
  } else {
    if (mag > static_cast<unsigned long long>(std::numeric_limits<std::int64_t>::max())) {
      return false;
    }
    out = static_cast<std::int64_t>(mag);
  }
  return true;
}

consteval auto pow10i(unsigned n) -> double {
  double v = 1.0;
  for (unsigned i = 0; i < n; ++i) {
    v *= 10.0;
  }
  return v;
}

consteval auto parseFloat64(std::string_view s, double& out) -> bool {
  if (s.empty()) {
    return false;
  }
  if (parseSpecialFloat(s, out)) {
    return true;
  }

  std::size_t pos = 0;
  bool        neg = false;
  if (s[pos] == '+' || s[pos] == '-') {
    neg = s[pos] == '-';
    ++pos;
  }
  if (pos >= s.size()) {
    return false;
  }

  auto const intStart = pos;
  while (pos < s.size() && (s[pos] == '_' || (s[pos] >= '0' && s[pos] <= '9'))) {
    ++pos;
  }
  auto const         intPart     = s.substr(intStart, pos - intStart);
  unsigned long long integerPart = 0;
  unsigned           intDigits   = 0;
  if (!parseUnderscoredUnsigned(intPart, 10, integerPart, intDigits)) {
    return false;
  }
  if (intDigits > 1 && intPart.front() == '0') {
    return false;
  }

  unsigned long long fracPart   = 0;
  unsigned           fracDigits = 0;
  bool               hasDot     = false;
  if (pos < s.size() && s[pos] == '.') {
    hasDot = true;
    ++pos;
    auto const fracStart = pos;
    while (pos < s.size() && (s[pos] == '_' || (s[pos] >= '0' && s[pos] <= '9'))) {
      ++pos;
    }
    auto const fracStr = s.substr(fracStart, pos - fracStart);
    if (!parseUnderscoredUnsigned(fracStr, 10, fracPart, fracDigits)) {
      return false;
    }
  }

  if (!hasDot && pos >= s.size()) {
    return false;
  }

  int  expSign = 1;
  int  exp     = 0;
  bool hasExp  = false;
  if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
    hasExp = true;
    ++pos;
    if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) {
      expSign = (s[pos] == '-') ? -1 : 1;
      ++pos;
    }
    if (pos >= s.size()) {
      return false;
    }
    auto const expStart = pos;
    while (pos < s.size() && (s[pos] == '_' || (s[pos] >= '0' && s[pos] <= '9'))) {
      ++pos;
    }
    unsigned long long expMag    = 0;
    unsigned           expDigits = 0;
    if (!parseUnderscoredUnsigned(s.substr(expStart, pos - expStart), 10, expMag, expDigits)) {
      return false;
    }
    if (expMag > static_cast<unsigned long long>(std::numeric_limits<int>::max())) {
      return false;
    }
    exp = static_cast<int>(expMag);
  }

  if (!hasDot && !hasExp) {
    return false;
  }

  if (pos != s.size()) {
    return false;
  }

  auto value = static_cast<double>(integerPart);
  if (fracDigits > 0) {
    value += static_cast<double>(fracPart) / pow10i(fracDigits);
  }
  if (exp != 0) {
    if (expSign > 0) {
      value *= pow10i(static_cast<unsigned>(exp));
    } else {
      value /= pow10i(static_cast<unsigned>(exp));
    }
  }
  out = neg ? -value : value;
  return true;
}

consteval auto hexDigitValue(char c) -> int {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

consteval auto parseHexN(std::string_view s, unsigned& out) -> bool {
  if (s.empty()) {
    return false;
  }
  unsigned value = 0;
  for (char const c: s) {
    auto const h = hexDigitValue(c);
    if (h < 0) {
      return false;
    }
    value = (value << 4U) | static_cast<unsigned>(h);
  }
  out = value;
  return true;
}

consteval auto appendUtf8Codepoint(std::string& out, unsigned cp) -> bool {
  if (cp > 0x10FFFFU) {
    return false;
  }
  if (cp >= 0xD800U && cp <= 0xDFFFU) {
    return false;
  }
  if (cp <= 0x7FU) {
    out.push_back(static_cast<char>(cp));
    return true;
  }
  if (cp <= 0x7FFU) {
    out.push_back(static_cast<char>(0xC0U | (cp >> 6U)));
    out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    return true;
  }
  if (cp <= 0xFFFFU) {
    out.push_back(static_cast<char>(0xE0U | (cp >> 12U)));
    out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    return true;
  }
  out.push_back(static_cast<char>(0xF0U | (cp >> 18U)));
  out.push_back(static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU)));
  out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
  out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
  return true;
}

consteval auto isDisallowedStringControl(char c, bool allowNewline) -> bool {
  auto const uc = static_cast<unsigned char>(c);
  if (uc == 0x09U) {
    return false;
  }
  if (allowNewline && c == '\n') {
    return false;
  }
  if (uc <= 0x1FU || uc == 0x7FU) {
    return true;
  }
  return false;
}

consteval auto consumeTomlStringToken(std::string_view sv, std::size_t start, bool allowMultiline, std::size_t& next)
  -> bool {
  if (start >= sv.size()) {
    return false;
  }
  auto const quote = sv[start];
  if (quote != '"' && quote != '\'') {
    return false;
  }
  bool const multiline = start + 2 < sv.size() && sv[start + 1] == quote && sv[start + 2] == quote;
  if (multiline && !allowMultiline) {
    return false;
  }
  std::size_t i = start + (multiline ? 3U : 1U);
  while (i < sv.size()) {
    auto const c = sv[i];
    if (!multiline && (c == '\n' || c == '\r')) {
      return false;
    }
    if (quote == '"' && c == '\\') {
      if (i + 1 >= sv.size()) {
        return false;
      }
      i += 2;
      continue;
    }
    if (multiline) {
      if (i + 2 < sv.size() && c == quote && sv[i + 1] == quote && sv[i + 2] == quote) {
        next = i + 3;
        return true;
      }
    } else if (c == quote) {
      next = i + 1;
      return true;
    }
    ++i;
  }
  return false;
}

consteval auto parseQuotedString(std::string_view in, std::string& out, bool allowMultiline = true) -> bool {
  if (in.size() < 2) {
    return false;
  }
  auto const quote = in.front();
  if ((quote != '"' && quote != '\'') || in.back() != quote) {
    return false;
  }

  bool const multiline =
    in.size() >= 6 && in[1] == quote && in[2] == quote && in[in.size() - 2] == quote && in[in.size() - 3] == quote;
  if (multiline && !allowMultiline) {
    return false;
  }
  if (!multiline && in.size() < 2) {
    return false;
  }

  auto body = multiline ? in.substr(3, in.size() - 6) : in.substr(1, in.size() - 2);
  if (multiline) {
    if (body.size() >= 2 && body[0] == '\r' && body[1] == '\n') {
      body.remove_prefix(2);
    } else if (!body.empty() && (body.front() == '\n' || body.front() == '\r')) {
      body.remove_prefix(1);
    }
  }

  out.clear();
  std::size_t i = 0;
  while (i < body.size()) {
    auto const c = body[i];
    if (quote == '"' && c == '\\') {
      if (i + 1 >= body.size()) {
        return false;
      }
      auto const esc = body[i + 1];
      if (multiline && (esc == '\n' || esc == '\r')) {
        i += 2;
        if (esc == '\r' && i < body.size() && body[i] == '\n') {
          ++i;
        }
        while (i < body.size() && (body[i] == ' ' || body[i] == '\t' || body[i] == '\n' || body[i] == '\r')) {
          ++i;
        }
        continue;
      }
      switch (esc) {
      case '\"':
        out.push_back('\"');
        i += 2;
        continue;
      case '\\':
        out.push_back('\\');
        i += 2;
        continue;
      case 'b':
        out.push_back('\b');
        i += 2;
        continue;
      case 't':
        out.push_back('\t');
        i += 2;
        continue;
      case 'n':
        out.push_back('\n');
        i += 2;
        continue;
      case 'f':
        out.push_back('\f');
        i += 2;
        continue;
      case 'r':
        out.push_back('\r');
        i += 2;
        continue;
      case 'e':
        out.push_back('\x1B');
        i += 2;
        continue;
      case 'x': {
        if (i + 3 >= body.size()) {
          return false;
        }
        unsigned v = 0;
        if (!parseHexN(body.substr(i + 2, 2), v)) {
          return false;
        }
        out.push_back(static_cast<char>(v));
        i += 4;
        continue;
      }
      case 'u': {
        if (i + 5 >= body.size()) {
          return false;
        }
        unsigned cp = 0;
        if (!parseHexN(body.substr(i + 2, 4), cp)) {
          return false;
        }
        if (!appendUtf8Codepoint(out, cp)) {
          return false;
        }
        i += 6;
        continue;
      }
      case 'U': {
        if (i + 9 >= body.size()) {
          return false;
        }
        unsigned cp = 0;
        if (!parseHexN(body.substr(i + 2, 8), cp)) {
          return false;
        }
        if (!appendUtf8Codepoint(out, cp)) {
          return false;
        }
        i += 10;
        continue;
      }
      default: return false;
      }
    }

    if (!multiline && (c == '\n' || c == '\r')) {
      return false;
    }
    if (multiline && c == '\r') {
      if (i + 1 < body.size() && body[i + 1] == '\n') {
        ++i;
      }
      out.push_back('\n');
      ++i;
      continue;
    }
    if (isDisallowedStringControl(c, multiline)) {
      return false;
    }
    out.push_back(c);
    ++i;
  }
  return true;
}

consteval auto parseLocalDate(std::string_view s, LocalDate& out) -> bool {
  if (s.size() != 10 || s[4] != '-' || s[7] != '-') {
    return false;
  }
  unsigned y = 0;
  unsigned m = 0;
  unsigned d = 0;
  if (!parseUnsigned(s.substr(0, 4), y) || !parseUnsigned(s.substr(5, 2), m) || !parseUnsigned(s.substr(8, 2), d)) {
    return false;
  }
  if (m == 0 || m > 12) {
    return false;
  }
  auto const isLeapYear = [](unsigned year) consteval {
    return (year % 4U == 0U) && ((year % 100U != 0U) || (year % 400U == 0U));
  };
  auto const maxDay = [&]() consteval -> unsigned {
    switch (m) {
    case 2 : return isLeapYear(y) ? 29U : 28U;
    case 4 :
    case 6 :
    case 9 :
    case 11: return 30U;
    default: return 31U;
    }
  }();
  if (d == 0 || d > maxDay) {
    return false;
  }
  out = LocalDate{static_cast<int>(y), m, d};
  return true;
}

consteval auto parseLocalTime(std::string_view s, LocalTime& out) -> bool {
  if (s.size() < 5 || s[2] != ':') {
    return false;
  }
  unsigned hh = 0;
  unsigned mm = 0;
  if (!parseUnsigned(s.substr(0, 2), hh) || !parseUnsigned(s.substr(3, 2), mm)) {
    return false;
  }
  if (hh > 23 || mm > 59) {
    return false;
  }

  std::size_t pos       = 5;
  unsigned    ss        = 0;
  bool        hasSecond = false;
  unsigned    nanos     = 0;
  if (pos < s.size()) {
    if (s[pos] != ':') {
      return false;
    }
    hasSecond = true;
    ++pos;
    if (pos + 2 > s.size()) {
      return false;
    }
    if (!parseUnsigned(s.substr(pos, 2), ss) || ss > 59) {
      return false;
    }
    pos += 2;
    if (pos < s.size()) {
      if (s[pos] != '.') {
        return false;
      }
      ++pos;
      if (pos >= s.size()) {
        return false;
      }
      unsigned           digits = 0;
      unsigned long long frac   = 0;
      while (pos < s.size()) {
        char const c = s[pos];
        if (c < '0' || c > '9') {
          return false;
        }
        if (digits < 9) {
          frac = frac * 10ULL + static_cast<unsigned>(c - '0');
          ++digits;
        }
        ++pos;
      }
      while (digits < 9) {
        frac *= 10ULL;
        ++digits;
      }
      nanos = static_cast<unsigned>(frac);
    }
  }
  out = LocalTime{hh, mm, ss, nanos, hasSecond};
  return true;
}

consteval auto parseDateOrDateTime(
  std::string_view s,
  LocalDate&       date,
  LocalTime&       time,
  bool&            hasTime,
  int&             offsetMinutes,
  bool&            hasOffset
) -> bool {
  hasTime       = false;
  hasOffset     = false;
  offsetMinutes = 0;
  if (!parseLocalDate(s.substr(0, 10), date)) {
    return false;
  }
  if (s.size() == 10) {
    return true;
  }
  if (s.size() < 16 || (s[10] != 'T' && s[10] != ' ')) {
    return false;
  }
  hasTime                   = true;
  auto             tail     = s.substr(11);
  auto             offPos   = tail.find_first_of("Zz+-");
  std::string_view timePart = tail;
  std::string_view offPart{};
  if (offPos != std::string_view::npos) {
    timePart = tail.substr(0, offPos);
    offPart  = tail.substr(offPos);
  }
  if (!parseLocalTime(timePart, time)) {
    return false;
  }
  if (offPart.empty()) {
    return true;
  }
  hasOffset = true;
  if (offPart == "Z" || offPart == "z") {
    offsetMinutes = 0;
    return true;
  }
  if (offPart.size() != 6 || (offPart[0] != '+' && offPart[0] != '-') || offPart[3] != ':') {
    return false;
  }
  unsigned oh = 0;
  unsigned om = 0;
  if (!parseUnsigned(offPart.substr(1, 2), oh) || !parseUnsigned(offPart.substr(4, 2), om)) {
    return false;
  }
  if (oh > 23 || om > 59) {
    return false;
  }
  auto total = static_cast<int>(oh * 60U + om);
  if (offPart[0] == '-') {
    total = -total;
  }
  offsetMinutes = total;
  return true;
}

consteval auto parseInlineTable(std::string_view raw) -> InlineParseResult;
consteval auto parseScalarArray(std::string_view raw) -> ArrayParseResult;

consteval auto isLegalMemberName(std::string_view key) -> bool {
  if (key.empty() || !isIdentStart(key.front())) {
    return false;
  }
  for (std::size_t i = 1; i < key.size(); ++i) {
    if (!isIdentContinue(key[i])) {
      return false;
    }
  }
  return true;
}

consteval auto shouldExcludeKey(std::string_view key) -> bool {
  auto const isCpp26Keyword = [](std::string_view value) consteval {
    constexpr auto keywords = std::array{
      "alignas",
      "alignof",
      "and",
      "and_eq",
      "asm",
      "auto",
      "bitand",
      "bitor",
      "bool",
      "break",
      "case",
      "catch",
      "char",
      "char8_t",
      "char16_t",
      "char32_t",
      "class",
      "compl",
      "concept",
      "const",
      "consteval",
      "constexpr",
      "constinit",
      "const_cast",
      "continue",
      "contract_assert",
      "co_await",
      "co_return",
      "co_yield",
      "decltype",
      "default",
      "delete",
      "do",
      "double",
      "dynamic_cast",
      "else",
      "enum",
      "explicit",
      "export",
      "extern",
      "false",
      "float",
      "for",
      "friend",
      "goto",
      "import",
      "if",
      "inline",
      "int",
      "long",
      "mutable",
      "module",
      "namespace",
      "new",
      "noexcept",
      "not",
      "not_eq",
      "nullptr",
      "operator",
      "or",
      "or_eq",
      "post",
      "pre",
      "private",
      "protected",
      "public",
      "register",
      "reflexpr",
      "reinterpret_cast",
      "replaceable_if_eligible",
      "requires",
      "return",
      "short",
      "signed",
      "sizeof",
      "static",
      "static_assert",
      "static_cast",
      "struct",
      "switch",
      "template",
      "this",
      "thread_local",
      "throw",
      "true",
      "trivially_relocatable_if_eligible",
      "try",
      "typedef",
      "typeid",
      "typename",
      "union",
      "unsigned",
      "using",
      "virtual",
      "void",
      "volatile",
      "wchar_t",
      "while",
      "xor",
      "xor_eq",
    };
    return std::ranges::find(keywords, value) != keywords.end();
  };

  auto const isMemberFunctionConflict = [](std::string_view value) consteval {
    constexpr auto conflicts = std::array{
      "begin",
      "end",
      "get",
      "indexLookup",
      "indices",
      "key",
      "lookupKey",
      "size",
      "staticSize",
    };
    return std::ranges::find(conflicts, value) != conflicts.end();
  };

  auto const isGeneratedCanonicalIndexName = [](std::string_view value) consteval {
    if (!value.starts_with("m_")) {
      return false;
    }
    auto const digits = value.substr(2);
    if (digits.empty()) {
      return false;
    }
    for (char c: digits) {
      if (c < '0' || c > '9') {
        return false;
      }
    }
    if (digits == "0") {
      return true;
    }
    return digits.front() >= '1' && digits.front() <= '9';
  };

  auto const hasDoubleUnderscore = [](std::string_view value) consteval {
    return value.find("__") != std::string_view::npos;
  };

  auto const startsWithUnderscoreUpper = [](std::string_view value) consteval {
    return value.size() >= 2 && value[0] == '_' && value[1] >= 'A' && value[1] <= 'Z';
  };

  return isCpp26Keyword(key)
      || isMemberFunctionConflict(key)
      || isGeneratedCanonicalIndexName(key)
      || hasDoubleUnderscore(key)
      || startsWithUnderscoreUpper(key);
}

consteval auto parseKeyToken(std::string& key) {
  return [&key](std::string_view sv) consteval -> std::string_view {
    if (sv.empty()) {
      throw ParseError::invalidKey;
    }
    if (sv.front() == '"' || sv.front() == '\'') {
      std::size_t rawEnd = 0;
      if (!consumeTomlStringToken(sv, 0, false, rawEnd)) {
        throw ParseError::invalidKey;
      }
      auto const  rawQuoted = sv.substr(0, rawEnd);
      std::string decoded;
      if (!parseQuotedString(rawQuoted, decoded, false)) {
        throw ParseError::invalidKey;
      }
      key = decoded;
      return sv.substr(rawEnd);
    }

    if (!isBareKeyChar(sv.front())) {
      throw ParseError::invalidKey;
    }
    auto i = std::size_t{1};
    while (i < sv.size() && isBareKeyChar(sv[i])) {
      ++i;
    }
    key.assign(sv.substr(0, i));
    return sv.substr(i);
  };
}

struct AppendPathSegmentStep {
  std::vector<std::string>* path = nullptr;
  std::string const*        key  = nullptr;

  consteval auto operator()(std::string_view in) const -> std::string_view {
    path->emplace_back(*key);
    return in;
  }
};

consteval auto appendPathSegment(std::vector<std::string>& path, std::string const& key) {
  return AppendPathSegmentStep{std::addressof(path), std::addressof(key)};
}

consteval auto parseKeyPath(std::vector<std::string>& path) {
  return [&path](std::string_view sv) consteval -> std::string_view {
    path.clear();
    auto hasDot = true;
    while (hasDot) {
      std::string key;
      sv = sv | parseKeyToken(key) | appendPathSegment(path, key)
             | wsInline()
             | consumeDot(hasDot)
             | applyIf(hasDot, wsInline())
             | applyIf(hasDot, expectNonEmpty(ParseError::invalidKey));
    }
    return sv;
  };
}

consteval auto pushDuplicateChecked(ParseOutput& out, std::string const& key, meta::info member, meta::info value)
  -> bool {
  for (auto const& k: out.keys) {
    if (k == key) {
      out.error = ParseError::duplicateKey;
      return false;
    }
  }
  out.keys.emplace_back(key);
  out.members.emplace_back(member);
  out.values.emplace_back(value);
  return true;
}

consteval auto containsMemberName(ParseOutput const& out, std::string_view memberName) -> bool {
  for (auto const& name: out.memberNames) {
    if (name == memberName) {
      return true;
    }
  }
  return false;
}

consteval auto assignMemberName(ParseOutput& out, std::string const& key) -> std::string {
  if (isLegalMemberName(key) && !shouldExcludeKey(key) && !containsMemberName(out, key)) {
    return key;
  }
  while (true) {
    auto candidate = makeArrayMemberName(out.hiddenNameIndex++);
    if (!containsMemberName(out, candidate) && candidate != key && !containsKey(out, candidate)) {
      return candidate;
    }
  }
}

consteval auto pushField(
  ParseOutput&             out,
  std::string const&       key,
  meta::info               memberType,
  meta::info               value,
  ValueType                valueType,
  bool                     inlineTable,
  std::vector<std::string> leadingComments  = {},
  std::vector<std::string> trailingComments = {}
) -> bool {
  for (auto const& k: out.keys) {
    if (k == key) {
      out.error = ParseError::duplicateKey;
      return false;
    }
  }
  auto const memberName = assignMemberName(out, key);
  auto const dms        = data_member_spec(memberType, {.name = memberName});
  out.keys.emplace_back(key);
  out.memberNames.emplace_back(memberName);
  out.types.emplace_back(valueType);
  out.inlineTables.emplace_back(inlineTable);
  out.leadingComments.emplace_back(std::move(leadingComments));
  out.trailingComments.emplace_back(std::move(trailingComments));
  out.members.emplace_back(meta::reflect_constant(dms));
  out.values.emplace_back(value);
  return true;
}

consteval auto wrapTableValue(ParseOutput const& out, meta::info baseValue) -> meta::info {
  std::vector<meta::info> tableTypeArgs{};
  tableTypeArgs.emplace_back(type_of(baseValue));
  for (auto const& key: out.keys) {
    tableTypeArgs.emplace_back(meta::reflect_constant_string(key));
  }
  auto                    tableType = substitute(^^TableObject, tableTypeArgs);
  std::vector<meta::info> wrappedArgs{};
  wrappedArgs.emplace_back(tableType);
  wrappedArgs.emplace_back(baseValue);
  return substitute(^^constructFrom, wrappedArgs);
}

consteval auto parseScalarValue(
  std::string const&       key,
  std::string_view         raw,
  ParseOutput&             out,
  std::vector<std::string> leadingComments  = {},
  std::vector<std::string> trailingComments = {}
) -> bool {
  if (raw.empty()) {
    out.error = ParseError::unsupportedValue;
    return false;
  }

  if (raw.front() == '{') {
    auto const parsed = parseInlineTable(raw);
    if (parsed.error != ParseError::none) {
      out.error = parsed.error;
      return false;
    }
    return pushField(
      out,
      key,
      type_of(parsed.value),
      parsed.value,
      ValueType::table,
      true,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw.front() == '[') {
    auto const parsed = parseScalarArray(raw);
    if (parsed.error != ParseError::none) {
      out.error = parsed.error;
      return false;
    }
    return pushField(
      out,
      key,
      parsed.type,
      parsed.value,
      ValueType::array,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw.front() == '"' || raw.front() == '\'') {
    std::string decoded;
    if (!parseQuotedString(raw, decoded)) {
      out.error = ParseError::invalidString;
      return false;
    }
    return pushField(
      out,
      key,
      ^^char const*,
      meta::reflect_constant_string(decoded),
      ValueType::string,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw == "true" || raw == "false") {
    auto const v = raw == "true";
    return pushField(
      out,
      key,
      ^^bool,
      meta::reflect_constant(v),
      ValueType::boolean,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw.size() >= 10 && raw[4] == '-' && raw[7] == '-') {
    LocalDate date{};
    LocalTime time{};
    bool      hasTime   = false;
    int       offset    = 0;
    bool      hasOffset = false;
    if (!parseDateOrDateTime(raw, date, time, hasTime, offset, hasOffset)) {
      out.error = (raw.size() == 10) ? ParseError::invalidDate : ParseError::invalidDateTime;
      return false;
    }
    if (!hasTime) {
      return pushField(
        out,
        key,
        ^^LocalDate,
        meta::reflect_constant(date),
        ValueType::localDate,
        false,
        std::move(leadingComments),
        std::move(trailingComments)
      );
    }
    if (hasOffset) {
      auto odt = OffsetDateTime{date, time, offset};
      return pushField(
        out,
        key,
        ^^OffsetDateTime,
        meta::reflect_constant(odt),
        ValueType::offsetDateTime,
        false,
        std::move(leadingComments),
        std::move(trailingComments)
      );
    }
    auto ldt = LocalDateTime{date, time};
    return pushField(
      out,
      key,
      ^^LocalDateTime,
      meta::reflect_constant(ldt),
      ValueType::localDateTime,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw.find(':') != std::string_view::npos) {
    LocalTime lt{};
    if (!parseLocalTime(raw, lt)) {
      out.error = ParseError::invalidTime;
      return false;
    }
    return pushField(
      out,
      key,
      ^^LocalTime,
      meta::reflect_constant(lt),
      ValueType::localTime,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  double f = 0.0;
  if (parseFloat64(raw, f)) {
    return pushField(
      out,
      key,
      ^^double,
      meta::reflect_constant(f),
      ValueType::floating,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  std::int64_t i = 0;
  if (parseInt64(raw, i)) {
    return pushField(
      out,
      key,
      ^^long long,
      meta::reflect_constant(i),
      ValueType::integer,
      false,
      std::move(leadingComments),
      std::move(trailingComments)
    );
  }

  if (raw.find_first_of(".eE") != std::string_view::npos
      || raw.find("inf") != std::string_view::npos
      || raw.find("nan") != std::string_view::npos) {
    out.error = ParseError::invalidFloat;
  } else {
    out.error = ParseError::invalidInteger;
  }
  return false;
}

consteval auto parseScalarArray(std::string_view raw) -> ArrayParseResult {
  ArrayParseResult              result{};
  ParseOutput                   local = makeParseOutput();
  std::vector<NamedTableOutput> localChildren{};
  auto                          in = raw;

  auto skipWsNl = [](std::string_view sv) consteval -> std::string_view {
    auto i = std::size_t{0};
    while (i < sv.size() && (isWs(sv[i]) || isNl(sv[i]))) {
      ++i;
    }
    return sv.substr(i);
  };

  auto skipCommentLine = [&](std::string_view sv) consteval -> std::string_view {
    if (sv.empty() || sv.front() != '#') {
      return sv;
    }
    auto i = std::size_t{1};
    while (i < sv.size() && sv[i] != '\r' && !isNl(sv[i])) {
      if (isDisallowedCommentControl(sv[i])) {
        result.error = ParseError::invalidComment;
        return {};
      }
      ++i;
    }
    if (i < sv.size() && sv[i] == '\r') {
      ++i;
      if (i < sv.size() && isNl(sv[i])) {
        ++i;
      }
      return sv.substr(i);
    }
    if (i < sv.size() && isNl(sv[i])) {
      ++i;
    }
    return sv.substr(i);
  };

  auto skipWsNlComments = [&](std::string_view sv) consteval -> std::string_view {
    while (true) {
      sv = skipWsNl(sv);
      if (!sv.empty() && sv.front() == '#') {
        sv = skipCommentLine(sv);
        continue;
      }
      return sv;
    }
  };

  auto materialize =
    [&](auto&& self, ParseOutput const& base, std::vector<NamedTableOutput> const& children) consteval -> meta::info {
    ParseOutput merged      = makeParseOutput();
    merged.members          = base.members;
    merged.values           = base.values;
    merged.keys             = base.keys;
    merged.memberNames      = base.memberNames;
    merged.types            = base.types;
    merged.inlineTables     = base.inlineTables;
    merged.leadingComments  = base.leadingComments;
    merged.trailingComments = base.trailingComments;
    merged.hiddenNameIndex  = base.hiddenNameIndex;
    for (auto const& child: children) {
      auto childValue = self(self, child.out, child.children);
      if (!pushField(merged, child.name, type_of(childValue), childValue, ValueType::table, false)) {
        result.error = merged.error;
        return ^^void;
      }
    }
    merged.values[0] = substitute(^^GeneratedAggregate, merged.members);
    auto baseValue   = substitute(^^constructFrom, merged.values);
    return wrapTableValue(merged, baseValue);
  };

  in = skipWsNlComments(in);
  if (in.empty() || in.front() != '[') {
    result.error = ParseError::invalidArray;
    return result;
  }
  in.remove_prefix(1);

  std::size_t idx = 0;
  while (true) {
    in = skipWsNlComments(in);
    if (in.empty()) {
      result.error = ParseError::invalidArray;
      return result;
    }
    if (in.front() == ']') {
      in.remove_prefix(1);
      break;
    }

    auto        i      = std::size_t{0};
    std::size_t bDepth = 0;
    std::size_t cDepth = 0;
    for (; i < in.size(); ++i) {
      auto const c = in[i];
      if (c == '"' || c == '\'') {
        std::size_t next = 0;
        if (!consumeTomlStringToken(in, i, true, next)) {
          result.error = ParseError::invalidString;
          return result;
        }
        i = next - 1;
        continue;
      }
      if (c == '[') {
        ++bDepth;
        continue;
      }
      if (c == ']') {
        if (bDepth == 0) {
          break;
        }
        --bDepth;
        continue;
      }
      if (c == '{') {
        ++cDepth;
        continue;
      }
      if (c == '}') {
        if (cDepth > 0) {
          --cDepth;
          continue;
        }
      }
      if (bDepth == 0 && cDepth == 0 && (c == ',' || c == '#')) {
        break;
      }
    }

    auto valueRaw = trimRight(in.substr(0, i));
    if (valueRaw.empty()) {
      result.error = ParseError::invalidArray;
      return result;
    }
    auto key = makeArrayMemberName(idx);
    if (!parseScalarValue(key, valueRaw, local)) {
      result.error = local.error;
      return result;
    }
    ++idx;

    in.remove_prefix(i);
    in = skipWsNl(in);
    if (!in.empty() && in.front() == '#') {
      in = skipCommentLine(in);
      in = skipWsNl(in);
    }

    if (in.empty()) {
      result.error = ParseError::invalidArray;
      return result;
    }
    if (in.front() == ',') {
      in.remove_prefix(1);
      continue;
    }
    if (in.front() == ']') {
      in.remove_prefix(1);
      break;
    }
    result.error = ParseError::invalidArray;
    return result;
  }

  in = skipWsNlComments(in);
  if (!in.empty()) {
    result.error = ParseError::invalidArray;
    return result;
  }

  result.value = materialize(materialize, local, localChildren);
  std::vector<meta::info> arrayTypeArgs{};
  arrayTypeArgs.emplace_back(type_of(result.value));
  result.type = substitute(^^ArrayObject, arrayTypeArgs);
  return result;
}

consteval auto parseInlineTable(std::string_view raw) -> InlineParseResult {
  InlineParseResult                     result{};
  ParseOutput                           local = makeParseOutput();
  std::vector<NamedTableOutput>         localChildren{};
  std::vector<std::vector<std::string>> sealedInlinePaths{};
  auto                                  in = raw;

  auto keyPathOf = [&](std::vector<std::string>& path) consteval {
    return [&path, &result](std::string_view sv) consteval -> std::string_view {
      auto rest = sv | onParseError(result.error, parseKeyPath(path));
      if (result.error != ParseError::none) {
        return {};
      }
      return rest;
    };
  };

  auto requireInline = [&](ParseError code) consteval {
    return [&, code](std::string_view sv) consteval -> std::string_view {
      if (sv.empty()) {
        result.error = code;
        return {};
      }
      return sv;
    };
  };

  auto expectInline = [&](char ch, ParseError code) consteval {
    return [&, ch, code](std::string_view sv) consteval -> std::string_view {
      if (sv.empty() || sv.front() != ch) {
        result.error = code;
        return {};
      }
      return sv.substr(1);
    };
  };

  auto valueSpan = [&](std::string_view& valueRaw) consteval {
    return [&](std::string_view sv) consteval -> std::string_view {
      auto i            = std::size_t{0};
      auto braceDepth   = std::size_t{0};
      auto bracketDepth = std::size_t{0};
      for (; i < sv.size(); ++i) {
        auto const c = sv[i];
        if (c == '"' || c == '\'') {
          std::size_t next = 0;
          if (!consumeTomlStringToken(sv, i, true, next)) {
            result.error = ParseError::invalidString;
            return {};
          }
          i = next - 1;
          continue;
        }
        if (c == '{') {
          ++braceDepth;
          continue;
        }
        if (c == '[') {
          ++bracketDepth;
          continue;
        }
        if (c == '}') {
          if (braceDepth == 0 && bracketDepth == 0) {
            break;
          }
          if (braceDepth > 0) {
            --braceDepth;
          }
          continue;
        }
        if (c == ']') {
          if (bracketDepth > 0) {
            --bracketDepth;
          }
          continue;
        }
        if (c == ',' && braceDepth == 0 && bracketDepth == 0) {
          break;
        }
      }
      valueRaw = trimRight(sv.substr(0, i));
      return sv.substr(i);
    };
  };

  auto insertScalarByPath = [&](std::vector<std::string> const& fullPath, std::string_view valueRaw) consteval {
    if (fullPath.empty()) {
      result.error = ParseError::invalidKey;
      return;
    }
    for (auto const& sealed: sealedInlinePaths) {
      if (startsWithPath(fullPath, sealed)) {
        result.error = ParseError::duplicateKey;
        return;
      }
    }
    auto* targetOut      = std::addressof(local);
    auto* targetChildren = std::addressof(localChildren);
    for (std::size_t i = 0; i + 1 < fullPath.size(); ++i) {
      auto* node = ensureChildTable(*targetOut, *targetChildren, fullPath[i], result.error);
      if (node == nullptr || result.error != ParseError::none) {
        return;
      }
      targetOut      = std::addressof(node->out);
      targetChildren = std::addressof(node->children);
    }
    auto const& leaf = fullPath.back();
    if (findTableByName(*targetChildren, leaf) != static_cast<std::size_t>(-1)) {
      result.error = ParseError::duplicateKey;
      return;
    }
    if (!parseScalarValue(leaf, valueRaw, *targetOut)) {
      if (targetOut->error != ParseError::none) {
        result.error = targetOut->error;
      }
      return;
    }
    auto const trimmed = trimRight(valueRaw);
    if (!trimmed.empty() && trimmed.front() == '{') {
      sealedInlinePaths.emplace_back(fullPath);
    }
  };

  in = in | wsLine() | expectInline('{', ParseError::invalidInlineTable);
  if (result.error != ParseError::none) {
    return result;
  }

  while (true) {
    in = in | wsLine() | requireInline(ParseError::invalidInlineTable);
    if (result.error != ParseError::none) {
      return result;
    }
    if (in.front() == '}') {
      in.remove_prefix(1);
      break;
    }

    std::vector<std::string> keyPath{};
    std::string_view         valueRaw{};
    in =
      in
      | keyPathOf(keyPath)
      | wsLine()
      | expectInline('=', ParseError::malformedLine)
      | wsLine()
      | requireInline(ParseError::unsupportedValue)
      | valueSpan(valueRaw);
    if (result.error != ParseError::none) {
      return result;
    }

    insertScalarByPath(keyPath, valueRaw);
    if (result.error != ParseError::none) {
      return result;
    }

    in = in | wsLine() | requireInline(ParseError::invalidInlineTable);
    if (result.error != ParseError::none) {
      return result;
    }
    if (in.front() == ',') {
      in.remove_prefix(1);
      continue;
    }
    if (in.front() == '}') {
      in.remove_prefix(1);
      break;
    }
    result.error = ParseError::invalidInlineTable;
    return result;
  }

  in = in | wsLine();
  if (!in.empty()) {
    result.error = ParseError::invalidInlineTable;
    return result;
  }

  auto materialize =
    [&](auto&& self, ParseOutput const& base, std::vector<NamedTableOutput> const& children) consteval -> meta::info {
    ParseOutput merged      = makeParseOutput();
    merged.members          = base.members;
    merged.values           = base.values;
    merged.keys             = base.keys;
    merged.memberNames      = base.memberNames;
    merged.types            = base.types;
    merged.inlineTables     = base.inlineTables;
    merged.leadingComments  = base.leadingComments;
    merged.trailingComments = base.trailingComments;
    merged.hiddenNameIndex  = base.hiddenNameIndex;
    for (auto const& child: children) {
      auto childValue = self(self, child.out, child.children);
      if (!pushField(merged, child.name, type_of(childValue), childValue, ValueType::table, false)) {
        result.error = merged.error;
        return ^^void;
      }
    }
    merged.values[0] = substitute(^^GeneratedAggregate, merged.members);
    auto baseValue   = substitute(^^constructFrom, merged.values);
    return wrapTableValue(merged, baseValue);
  };

  result.value = materialize(materialize, local, localChildren);
  return result;
}

consteval auto parseRootKv(std::string_view src) -> ParseOutput {
  ParseOutput                           out = makeParseOutput();
  std::vector<NamedTableOutput>         tables{};
  std::vector<std::string>              currentTablePath{};
  ParseOutput*                          currentOut      = std::addressof(out);
  std::vector<NamedTableOutput>*        currentChildren = std::addressof(tables);
  std::vector<std::vector<std::string>> sealedInlinePaths{};
  std::vector<std::string>              pendingComments{};
  std::vector<std::string>              rootLeadingComments{};
  std::vector<std::string>              rootTrailingComments{};
  auto                                  rest = src;

  auto eof = [&]() consteval -> bool { return rest.empty() || out.error != ParseError::none; };

  auto skipInline = []() consteval {
    return [](std::string_view sv) consteval -> std::string_view {
      auto i = std::size_t{0};
      while (i < sv.size() && isWs(sv[i])) {
        ++i;
      }
      return sv.substr(i);
    };
  };

  auto skipLeading = []() consteval {
    return [](std::string_view sv) consteval -> std::string_view {
      auto i = std::size_t{0};
      while (i < sv.size() && (isWs(sv[i]) || isNl(sv[i]))) {
        ++i;
      }
      return sv.substr(i);
    };
  };

  auto consumeNewline = []() consteval {
    return [](std::string_view sv) consteval -> std::string_view {
      if (!sv.empty() && isNl(sv.front())) {
        return sv.substr(1);
      }
      return sv;
    };
  };

  auto parseCommentLine = [&](std::string_view sv, std::string& comment) consteval -> std::string_view {
    comment.clear();
    if (sv.empty() || sv.front() != '#') {
      return sv;
    }
    auto i = std::size_t{1};
    while (i < sv.size() && sv[i] != '\r' && !isNl(sv[i])) {
      if (isDisallowedCommentControl(sv[i])) {
        out.error = ParseError::invalidComment;
        return {};
      }
      comment.push_back(sv[i]);
      ++i;
    }
    if (i < sv.size() && sv[i] == '\r') {
      ++i;
      if (i < sv.size() && isNl(sv[i])) {
        ++i;
      }
      return sv.substr(i);
    }
    if (i < sv.size() && isNl(sv[i])) {
      ++i;
    }
    return sv.substr(i);
  };

  auto expectChar = [&](char c, ParseError error) consteval {
    return [=, &out](std::string_view sv) consteval -> std::string_view {
      if (sv.empty() || sv.front() != c) {
        out.error = error;
        return {};
      }
      return sv.substr(1);
    };
  };

  auto parsePath = [&](std::vector<std::string>& path) consteval {
    return [&path, &out](std::string_view sv) consteval -> std::string_view {
      return sv | onParseError(out.error, parseKeyPath(path));
    };
  };

  auto parseHeader = [&](std::vector<std::string>& tablePath, bool& arrayHeader) consteval {
    return [&tablePath, &arrayHeader, &out, &skipInline](std::string_view sv) consteval -> std::string_view {
      if (sv.empty() || sv.front() != '[') {
        out.error = ParseError::malformedLine;
        return {};
      }
      arrayHeader = sv.size() >= 2 && sv[1] == '[';
      sv          = sv.substr(arrayHeader ? 2 : 1);
      sv          = sv | skipInline();

      sv = sv | onParseError(out.error, parseKeyPath(tablePath));
      if (out.error != ParseError::none) {
        return {};
      }

      sv = sv | skipInline();
      if (arrayHeader) {
        if (sv.size() < 2 || sv[0] != ']' || sv[1] != ']') {
          out.error = ParseError::malformedLine;
          return {};
        }
        return sv.substr(2);
      }
      if (sv.empty() || sv.front() != ']') {
        out.error = ParseError::malformedLine;
        return {};
      }
      return sv.substr(1);
    };
  };

  auto parsePlain = [&](std::string_view& raw) consteval {
    return [&raw, &out](std::string_view sv) consteval -> std::string_view {
      auto i            = std::size_t{0};
      auto braceDepth   = std::size_t{0};
      auto bracketDepth = std::size_t{0};
      for (; i < sv.size(); ++i) {
        auto const c = sv[i];
        if (c == '"' || c == '\'') {
          std::size_t next = 0;
          if (!consumeTomlStringToken(sv, i, true, next)) {
            out.error = ParseError::invalidString;
            return {};
          }
          i = next - 1;
          continue;
        }
        if (c == '{') {
          ++braceDepth;
          continue;
        }
        if (c == '[') {
          ++bracketDepth;
          continue;
        }
        if (c == '}') {
          if (braceDepth > 0) {
            --braceDepth;
            continue;
          }
        }
        if (c == ']') {
          if (bracketDepth > 0) {
            --bracketDepth;
            continue;
          }
        }
        if (braceDepth == 0 && bracketDepth == 0 && (isNl(c) || c == '#')) {
          break;
        }
      }
      raw = trimRight(sv.substr(0, i));
      return sv.substr(i);
    };
  };

  auto ensureLineEnd = [&](ParseError error) consteval {
    return [=, &out](std::string_view sv) consteval -> std::string_view {
      if (!sv.empty() && !isNl(sv.front()) && sv.front() != '#') {
        out.error = error;
        return {};
      }
      return sv;
    };
  };

  auto findChild = [](std::vector<NamedTableOutput>& children, std::string_view key) consteval -> NamedTableOutput* {
    auto const idx = findTableByName(children, key);
    if (idx == static_cast<std::size_t>(-1)) {
      return nullptr;
    }
    return std::addressof(children[idx]);
  };

  auto getOrCreateChild =
    [&](ParseOutput& parentOut, std::vector<NamedTableOutput>& children, std::string_view key) consteval
    -> NamedTableOutput* {
    if (containsKey(parentOut, key)) {
      out.error = ParseError::duplicateKey;
      return nullptr;
    }
    if (auto* node = findChild(children, key); node != nullptr) {
      return node;
    }
    NamedTableOutput entry{};
    entry.name = std::string{key};
    entry.out  = makeParseOutput();
    children.emplace_back(entry);
    return std::addressof(children.back());
  };

  auto applyHeader =
    [&](
      std::vector<std::string> const& logicalPath,
      bool                            arrayHeader,
      std::vector<std::string>        leadingComments,
      std::vector<std::string>        trailingComments
    ) consteval {
      if (logicalPath.empty()) {
        out.error = ParseError::invalidKey;
        return;
      }

      auto*                    targetOut      = std::addressof(out);
      auto*                    targetChildren = std::addressof(tables);
      std::vector<std::string> physicalPath{};

      auto descendIntermediate = [&](std::string_view seg) consteval -> bool {
        auto* node = getOrCreateChild(*targetOut, *targetChildren, seg);
        if (node == nullptr || out.error != ParseError::none) {
          return false;
        }
        if (node->arrayContainer) {
          if (node->children.empty()) {
            out.error = ParseError::duplicateKey;
            return false;
          }
          auto* elem = std::addressof(node->children.back());
          physicalPath.emplace_back(std::string{seg});
          physicalPath.emplace_back(elem->name);
          targetOut      = std::addressof(elem->out);
          targetChildren = std::addressof(elem->children);
          return true;
        }
        physicalPath.emplace_back(std::string{seg});
        targetOut      = std::addressof(node->out);
        targetChildren = std::addressof(node->children);
        return true;
      };

      if (arrayHeader) {
        for (std::size_t i = 0; i + 1 < logicalPath.size(); ++i) {
          if (!descendIntermediate(logicalPath[i])) {
            return;
          }
        }
        auto const& leaf = logicalPath.back();
        if (containsKey(*targetOut, leaf)) {
          out.error = ParseError::duplicateKey;
          return;
        }

        auto* container = findChild(*targetChildren, leaf);
        if (container == nullptr) {
          NamedTableOutput entry{};
          entry.name           = leaf;
          entry.out            = makeParseOutput();
          entry.arrayContainer = true;
          targetChildren->emplace_back(entry);
          container = std::addressof(targetChildren->back());
        } else if (!container->arrayContainer) {
          out.error = ParseError::duplicateKey;
          return;
        }

        physicalPath.emplace_back(leaf);
        for (auto const& sealed: sealedInlinePaths) {
          if (startsWithPath(physicalPath, sealed)) {
            out.error = ParseError::duplicateKey;
            return;
          }
        }

        NamedTableOutput element{};
        element.name             = makeArrayMemberName(container->nextArrayIndex++);
        element.out              = makeParseOutput();
        element.explicitHeader   = true;
        element.leadingComments  = std::move(leadingComments);
        element.trailingComments = std::move(trailingComments);
        container->children.emplace_back(element);
        auto* elem = std::addressof(container->children.back());

        physicalPath.emplace_back(elem->name);
        currentTablePath = physicalPath;
        currentOut       = std::addressof(elem->out);
        currentChildren  = std::addressof(elem->children);
        return;
      }

      for (std::size_t i = 0; i + 1 < logicalPath.size(); ++i) {
        if (!descendIntermediate(logicalPath[i])) {
          return;
        }
      }

      auto const&       leaf = logicalPath.back();
      NamedTableOutput* node = getOrCreateChild(*targetOut, *targetChildren, leaf);
      if (node == nullptr || out.error != ParseError::none) {
        return;
      }
      if (node->arrayContainer) {
        out.error = ParseError::duplicateKey;
        return;
      }

      physicalPath.emplace_back(leaf);
      for (auto const& sealed: sealedInlinePaths) {
        if (startsWithPath(physicalPath, sealed)) {
          out.error = ParseError::duplicateKey;
          return;
        }
      }
      if (node->explicitHeader || node->dottedDefined) {
        out.error = ParseError::duplicateKey;
        return;
      }

      node->explicitHeader   = true;
      node->leadingComments  = std::move(leadingComments);
      node->trailingComments = std::move(trailingComments);
      currentTablePath       = physicalPath;
      currentOut             = std::addressof(node->out);
      currentChildren        = std::addressof(node->children);
    };

  auto insertScalarRelative =
    [&](
      std::vector<std::string> const& keyPath,
      std::string_view                raw,
      std::vector<std::string>        leadingComments,
      std::vector<std::string>        trailingComments
    ) consteval {
      if (keyPath.empty()) {
        out.error = ParseError::invalidKey;
        return;
      }

      auto*                    targetOut      = currentOut;
      auto*                    targetChildren = currentChildren;
      std::vector<std::string> fullPath       = currentTablePath;

      for (std::size_t i = 0; i + 1 < keyPath.size(); ++i) {
        auto const&       seg  = keyPath[i];
        NamedTableOutput* node = getOrCreateChild(*targetOut, *targetChildren, seg);
        if (node == nullptr || out.error != ParseError::none) {
          return;
        }
        if (node->arrayContainer) {
          if (node->children.empty()) {
            out.error = ParseError::duplicateKey;
            return;
          }
          auto* elem          = std::addressof(node->children.back());
          elem->dottedDefined = true;
          fullPath.emplace_back(seg);
          fullPath.emplace_back(elem->name);
          targetOut      = std::addressof(elem->out);
          targetChildren = std::addressof(elem->children);
        } else {
          node->dottedDefined = true;
          fullPath.emplace_back(seg);
          targetOut      = std::addressof(node->out);
          targetChildren = std::addressof(node->children);
        }
      }

      auto const& leaf = keyPath.back();
      fullPath.emplace_back(leaf);
      for (auto const& sealed: sealedInlinePaths) {
        if (startsWithPath(fullPath, sealed)) {
          out.error = ParseError::duplicateKey;
          return;
        }
      }
      if (findTableByName(*targetChildren, leaf) != static_cast<std::size_t>(-1)) {
        out.error = ParseError::duplicateKey;
        return;
      }
      if (!parseScalarValue(leaf, raw, *targetOut, std::move(leadingComments), std::move(trailingComments))) {
        if (targetOut->error != ParseError::none) {
          out.error = targetOut->error;
        }
        return;
      }
      auto const trimmed = trimRight(raw);
      if (!trimmed.empty() && trimmed.front() == '{') {
        sealedInlinePaths.emplace_back(fullPath);
      }
    };

  while (!eof()) {
    rest = rest | skipLeading();
    if (eof()) {
      break;
    }
    if (!rest.empty() && rest.front() == '#') {
      std::string commentText{};
      rest = parseCommentLine(rest, commentText);
      if (out.error != ParseError::none) {
        break;
      }
      pendingComments.emplace_back(std::move(commentText));
      continue;
    }

    if (!rest.empty() && rest.front() == '[') {
      std::vector<std::string> tablePath{};
      bool                     arrayHeader = false;
      rest = rest | parseHeader(tablePath, arrayHeader) | skipInline() | ensureLineEnd(ParseError::malformedLine);
      if (out.error != ParseError::none) {
        break;
      }
      std::vector<std::string> trailingComments{};
      if (!rest.empty() && rest.front() == '#') {
        std::string commentText{};
        rest = parseCommentLine(rest, commentText);
        if (out.error != ParseError::none) {
          break;
        }
        trailingComments.emplace_back(std::move(commentText));
      } else {
        rest = rest | consumeNewline();
      }
      applyHeader(tablePath, arrayHeader, std::move(pendingComments), std::move(trailingComments));
      pendingComments.clear();
      if (out.error != ParseError::none) {
        break;
      }
      continue;
    }

    std::vector<std::string> keyPath{};
    std::string_view         raw{};
    rest = rest | parsePath(keyPath) | skipInline() | expectChar('=', ParseError::malformedLine) | skipInline();
    if (out.error != ParseError::none) {
      break;
    }
    rest = rest | parsePlain(raw);
    if (out.error != ParseError::none) {
      break;
    }

    rest = rest | skipInline() | ensureLineEnd(ParseError::malformedLine);
    if (out.error != ParseError::none) {
      break;
    }
    std::vector<std::string> trailingComments{};
    if (!rest.empty() && rest.front() == '#') {
      std::string commentText{};
      rest = parseCommentLine(rest, commentText);
      if (out.error != ParseError::none) {
        break;
      }
      trailingComments.emplace_back(std::move(commentText));
    } else {
      rest = rest | consumeNewline();
    }

    insertScalarRelative(keyPath, raw, std::move(pendingComments), std::move(trailingComments));
    pendingComments.clear();
    if (out.error != ParseError::none) {
      break;
    }
  }

  if (out.error == ParseError::none && !pendingComments.empty()) {
    rootTrailingComments = std::move(pendingComments);
  }

  if (out.error != ParseError::none) {
    return out;
  }

  auto appendKeyPath = [](std::string const& base, std::string_view key) consteval -> std::string {
    if (base == "$") {
      return std::string{key};
    }
    std::string outPath = base;
    outPath.push_back('.');
    outPath.append(key);
    return outPath;
  };

  auto appendIndexPath = [](std::string const& base, std::size_t idx) consteval -> std::string {
    auto        outPath = base;
    std::string reversed{};
    if (idx == 0) {
      reversed.push_back('0');
    } else {
      while (idx > 0) {
        reversed.push_back(static_cast<char>('0' + (idx % 10)));
        idx /= 10;
      }
    }
    outPath.push_back('[');
    for (auto it = reversed.rbegin(); it != reversed.rend(); ++it) {
      outPath.push_back(*it);
    }
    outPath.push_back(']');
    return outPath;
  };

  auto appendComments = [&](std::vector<std::string> const& comments) consteval -> std::pair<std::size_t, std::size_t> {
    auto const offset = out.comments.size();
    for (auto const& c: comments) {
      out.comments.emplace_back(c);
    }
    return {offset, comments.size()};
  };

  auto pushMetaEntry =
    [&](
      std::string                     path,
      ValueType                       type,
      bool                            inlineTable,
      bool                            explicitHeader,
      bool                            dottedDefined,
      bool                            arrayContainer,
      std::vector<std::string> const& leadingComments,
      std::vector<std::string> const& trailingComments
    ) consteval {
      ParseOutput::MetaEntrySpec entry{};
      auto const [leadingOffset, leadingCount]   = appendComments(leadingComments);
      auto const [trailingOffset, trailingCount] = appendComments(trailingComments);
      entry.path                                 = std::move(path);
      entry.type                                 = type;
      entry.inlineTable                          = inlineTable;
      entry.explicitHeader                       = explicitHeader;
      entry.dottedDefined                        = dottedDefined;
      entry.arrayContainer                       = arrayContainer;
      entry.leadingOffset                        = leadingOffset;
      entry.leadingCount                         = leadingCount;
      entry.trailingOffset                       = trailingOffset;
      entry.trailingCount                        = trailingCount;
      out.metaEntries.emplace_back(std::move(entry));
    };

  auto collectMeta =
    [&](
      auto&&                               self,
      ParseOutput const&                   base,
      std::vector<NamedTableOutput> const& children,
      std::string const&                   tablePath,
      bool                                 explicitHeader,
      bool                                 dottedDefined,
      bool                                 arrayContainer,
      std::vector<std::string> const&      tableLeadingComments,
      std::vector<std::string> const&      tableTrailingComments
    ) consteval -> void {
    pushMetaEntry(
      tablePath,
      ValueType::table,
      false,
      explicitHeader,
      dottedDefined,
      arrayContainer,
      tableLeadingComments,
      tableTrailingComments
    );

    for (std::size_t i = 0; i < base.keys.size(); ++i) {
      auto const  type        = (i < base.types.size()) ? base.types[i] : ValueType::none;
      auto const  inlineTable = (i < base.inlineTables.size()) ? base.inlineTables[i] : false;
      auto const  empty       = std::vector<std::string>{};
      auto const& leading     = (i < base.leadingComments.size()) ? base.leadingComments[i] : empty;
      auto const& trailing    = (i < base.trailingComments.size()) ? base.trailingComments[i] : empty;
      pushMetaEntry(appendKeyPath(tablePath, base.keys[i]), type, inlineTable, false, false, false, leading, trailing);
    }

    for (std::size_t i = 0; i < children.size(); ++i) {
      auto const& child     = children[i];
      auto const  childPath = appendKeyPath(tablePath, child.name);
      if (child.arrayContainer) {
        pushMetaEntry(
          childPath, ValueType::array, false, false, false, true, child.leadingComments, child.trailingComments
        );
        for (std::size_t j = 0; j < child.children.size(); ++j) {
          auto const& elem     = child.children[j];
          auto const  elemPath = appendIndexPath(childPath, j);
          self(
            self,
            elem.out,
            elem.children,
            elemPath,
            elem.explicitHeader,
            elem.dottedDefined,
            false,
            elem.leadingComments,
            elem.trailingComments
          );
        }
      } else {
        self(
          self,
          child.out,
          child.children,
          childPath,
          child.explicitHeader,
          child.dottedDefined,
          false,
          child.leadingComments,
          child.trailingComments
        );
      }
    }
  };

  collectMeta(collectMeta, out, tables, "$", true, false, false, rootLeadingComments, rootTrailingComments);

  auto materialize =
    [&](auto&& self, ParseOutput const& base, std::vector<NamedTableOutput> const& children) consteval -> meta::info {
    ParseOutput merged      = makeParseOutput();
    merged.members          = base.members;
    merged.values           = base.values;
    merged.keys             = base.keys;
    merged.memberNames      = base.memberNames;
    merged.types            = base.types;
    merged.inlineTables     = base.inlineTables;
    merged.leadingComments  = base.leadingComments;
    merged.trailingComments = base.trailingComments;
    merged.hiddenNameIndex  = base.hiddenNameIndex;
    for (auto const& child: children) {
      if (child.arrayContainer) {
        ParseOutput arrOut = makeParseOutput();
        for (std::size_t i = 0; i < child.children.size(); ++i) {
          auto elemValue = self(self, child.children[i].out, child.children[i].children);
          if (out.error != ParseError::none) {
            return ^^void;
          }
          auto const memberName = makeArrayMemberName(i);
          if (!pushField(arrOut, memberName, type_of(elemValue), elemValue, ValueType::table, false)) {
            out.error = arrOut.error;
            return ^^void;
          }
        }
        arrOut.values[0]                    = substitute(^^GeneratedAggregate, arrOut.members);
        auto                    arrRepValue = substitute(^^constructFrom, arrOut.values);
        auto                    arrPacked   = wrapTableValue(arrOut, arrRepValue);
        std::vector<meta::info> arrayTypeArgs{};
        arrayTypeArgs.emplace_back(type_of(arrPacked));
        auto arrType = substitute(^^ArrayObject, arrayTypeArgs);
        if (!pushField(merged, child.name, arrType, arrPacked, ValueType::array, false)) {
          out.error = merged.error;
          return ^^void;
        }
      } else {
        auto childValue = self(self, child.out, child.children);
        if (out.error != ParseError::none) {
          return ^^void;
        }
        if (!pushField(merged, child.name, type_of(childValue), childValue, ValueType::table, false)) {
          out.error = merged.error;
          return ^^void;
        }
      }
    }
    merged.values[0] = substitute(^^GeneratedAggregate, merged.members);
    auto baseValue   = substitute(^^constructFrom, merged.values);
    return wrapTableValue(merged, baseValue);
  };

  for (auto const& table: tables) {
    auto tableValue = materialize(materialize, table.out, table.children);
    if (out.error != ParseError::none) {
      return out;
    }
    if (!pushField(out, table.name, type_of(tableValue), tableValue, ValueType::table, false)) {
      return out;
    }
  }

  return out;
}

template<ParseError E>
consteval auto failParse() -> void {
  if constexpr (E == ParseError::malformedLine) {
    static constexpr std::string msg = "E_MLINE";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidNewline) {
    static constexpr std::string msg = "E_INL";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidComment) {
    static constexpr std::string msg = "E_ICOM";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidUtf8) {
    static constexpr std::string msg = "E_UTF8";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidKey) {
    static constexpr std::string msg = "E_IKEY";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::duplicateKey) {
    static constexpr std::string msg = "E_DKEY";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidArray) {
    static constexpr std::string msg = "E_IARR";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidInlineTable) {
    static constexpr std::string msg = "E_IINL";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidString) {
    static constexpr std::string msg = "E_ISTR";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidInteger) {
    static constexpr std::string msg = "E_IINT";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidFloat) {
    static constexpr std::string msg = "E_IFLT";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidBoolean) {
    static constexpr std::string msg = "E_IBOOL";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidDate) {
    static constexpr std::string msg = "E_IDATE";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidTime) {
    static constexpr std::string msg = "E_ITIME";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::invalidDateTime) {
    static constexpr std::string msg = "E_IDT";
    static_assert(false, msg);
  } else if constexpr (E == ParseError::unsupportedValue) {
    static constexpr std::string msg = "E_UNSUP";
    static_assert(false, msg);
  } else {
    static constexpr std::string msg = "E_UNK";
    static_assert(false, msg);
  }
}

template<ParseError E>
consteval auto failParseValue() -> int {
  failParse<E>();
  return 0;
}

consteval auto normalizeSourceView(std::string_view source) -> std::string_view {
  auto normalized = source;
  if (!normalized.empty() && normalized.back() == '\0') {
    normalized.remove_suffix(1);
  }
  return normalized;
}

template<FixedString Source>
consteval auto parseErrorOfText() -> ParseError {
  auto const normalized = normalizeSourceView(Source.view());
  if (!hasOnlyLfOrCrlf(normalized)) {
    return ParseError::invalidNewline;
  }
  if (!isWellFormedUtf8(normalized)) {
    return ParseError::invalidUtf8;
  }
  auto const out = parseRootKv(normalized);
  return out.error;
}

template<auto SourceBytes>
consteval auto parseErrorOfBytes() -> ParseError {
  constexpr std::string_view sourceView{SourceBytes};
  auto const                 normalized = normalizeSourceView(sourceView);
  if (!hasOnlyLfOrCrlf(normalized)) {
    return ParseError::invalidNewline;
  }
  if (!isWellFormedUtf8(normalized)) {
    return ParseError::invalidUtf8;
  }
  auto const out = parseRootKv(normalized);
  return out.error;
}
}  // namespace detail

using detail::operator|;

template<class Pred>
struct ThrowIfStep {
  Pred        pred{};
  std::string message{};

  consteval auto operator()(std::string_view sv) const -> std::string_view {
    if (pred(sv)) {
      throw message;
    }
    return sv;
  }
};

consteval auto hasInvalidNewline(std::string_view sv) -> bool { return !detail::hasOnlyLfOrCrlf(sv); }

consteval auto hasInvalidUtf8(std::string_view sv) -> bool { return !detail::isWellFormedUtf8(sv); }

template<class Pred>
consteval auto throwIf(Pred pred, std::string message) {
  return ThrowIfStep<Pred>{pred, std::move(message)};
}

consteval auto parseChecked(std::string_view source, std::string message) -> detail::ParseOutput {
  auto const out = detail::parseRootKv(source);
  if (out.error != detail::ParseError::none) {
    throw message;
  }
  return out;
}

consteval auto parseAsReflection(std::string_view source) -> meta::info {
  auto normalized = detail::normalizeSourceView(source);
  normalized =
    normalized
    | throwIf(hasInvalidNewline, std::string{"parseAsReflection: invalid newline"})
    | throwIf(hasInvalidUtf8, std::string{"parseAsReflection: invalid utf8"});
  auto out                          = parseChecked(normalized, std::string{"parseAsReflection: parse failed"});
  auto values                       = out.values;
  values[0]                         = substitute(^^GeneratedAggregate, out.members);
  auto                    baseValue = substitute(^^constructFrom, values);
  auto                    wrapped   = detail::wrapTableValue(out, baseValue);
  std::vector<meta::info> rootArgs{
    type_of(wrapped),
    wrapped,
  };
  return substitute(^^constructRoot, rootArgs);
}

consteval auto parseMetaAsReflection(std::string_view source) -> meta::info {
  auto normalized = detail::normalizeSourceView(source);
  normalized =
    normalized
    | throwIf(hasInvalidNewline, std::string{"parseMetaAsReflection: invalid newline"})
    | throwIf(hasInvalidUtf8, std::string{"parseMetaAsReflection: invalid utf8"});
  auto                    out = parseChecked(normalized, std::string{"parseMetaAsReflection: parse failed"});
  std::vector<meta::info> entryValues{};
  entryValues.reserve(out.metaEntries.size());
  for (auto const& entry: out.metaEntries) {
    std::vector<meta::info> args{
      meta::reflect_constant_string(entry.path),
      meta::reflect_constant(entry.type),
      meta::reflect_constant(entry.inlineTable),
      meta::reflect_constant(entry.explicitHeader),
      meta::reflect_constant(entry.dottedDefined),
      meta::reflect_constant(entry.arrayContainer),
      meta::reflect_constant(entry.leadingOffset),
      meta::reflect_constant(entry.leadingCount),
      meta::reflect_constant(entry.trailingOffset),
      meta::reflect_constant(entry.trailingCount),
    };
    entryValues.emplace_back(substitute(^^constructMetaEntry, args));
  }
  if (entryValues.empty()) {
    throw std::string{"parseMetaAsReflection: empty metadata"};
  }
  return substitute(^^constructMetaArray, entryValues);
}

consteval auto parseCommentsAsReflection(std::string_view source) -> meta::info {
  auto normalized = detail::normalizeSourceView(source);
  normalized =
    normalized
    | throwIf(hasInvalidNewline, std::string{"parseCommentsAsReflection: invalid newline"})
    | throwIf(hasInvalidUtf8, std::string{"parseCommentsAsReflection: invalid utf8"});
  auto out = parseChecked(normalized, std::string{"parseCommentsAsReflection: parse failed"});
  if (out.comments.empty()) {
    return ^^constructEmptyCommentArray;
  }
  std::vector<meta::info> commentValues{};
  commentValues.reserve(out.comments.size());
  for (auto const& comment: out.comments) {
    commentValues.emplace_back(meta::reflect_constant_string(comment));
  }
  return substitute(^^constructCommentArray, commentValues);
}

template<std::size_t EntryCount>
consteval auto findRootMetaIndex(std::array<MetaEntry, EntryCount> const& entries) -> std::size_t {
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (std::string_view{entries[i].path} == "$") {
      return i;
    }
  }
  return 0;
}

#endif



