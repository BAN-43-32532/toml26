# tomlct

Compile-time TOML support for C++.

## TODO

### Compile-time TOML parsing
Support `consteval` TOML parsing from:
- string literals
- `#embed`-embedded TOML sources

Construction should be possible via:
- a `_toml` user-defined literal
- an explicit API such as `tomlct::parse(...)` (tentative)

Parsing is performed at compile time and should transparently fall back to runtime when needed.  
PoC: https://godbolt.org/z/3jzehW1bv

---

### Feature parity with toml11
Target API design and TOML coverage comparable to **toml11**:  
https://github.com/ToruNiina/toml11

This includes full syntax support and preservation of formatting-related information
(e.g. comments).

---

### Reflection-based struct construction
Prioritize struct construction via compile-time reflection.

---

### Compile-time TOML metadata
Store TOML metadata at compile time for formatting and tooling.

---

### TOML serialization
Support writing TOML from in-memory representations.

---

### Runtime support and pre-C++26 compatibility
Provide a runtime parsing path and an alternative storage backend
(e.g. `std::map`-based) for environments without C++26 support.
