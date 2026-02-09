#toml26 Test Notes

This test tree uses a simple naming convention:

- `pass_*`: must compile and run with exit code `0`.
- `fail_*`: must fail at compile time (`run_fail_case.cmake` checks compile failure only).

## Current Coverage

1. Root key/value parsing and runtime access
- `pass_root_basic`

2. Dotted keys and path conflicts
- `pass_dotted_keys`
- `pass_key_bare_syntax`
- `fail_dotted_header_conflict`
- `fail_duplicate_key`
- `fail_key_invalid_bare_char`

3. Table semantics and inline-table closure rules
- `pass_inline_table`
- `pass_table_semantics`
- `fail_inline_table_closed`

4. Numeric rules
- `pass_numeric_rules`
- `fail_numeric_invalid_integer`
- `fail_numeric_invalid_float`

5. UTF-8 baseline validation
- `pass_utf8_validation`

6. Newline policy (`LF` / `CRLF` only)
- `pass_line_ending_crlf`
- `fail_line_ending_cr_only`

7. Comment control-character validation
- `pass_comment_control_chars`
- `fail_comment_control_char`

8. Compile-time / runtime traversal
- `pass_for_template_for`
- `pass_for_range_for`

9. Runtime `at` / `at_or` / `as_or`
- `pass_at_basic`
- `pass_at_or_missing`
- `pass_at_or_type_mismatch`
- `pass_value_ref_as_or`
- `pass_find_optional`

10. TOML -> JSON serialization
- `pass_to_json`

11. String rules
- `pass_string_basic_escapes`
- `pass_string_literal_single`
- `pass_string_multiline_basic`
- `pass_string_multiline_literal`
- `pass_string_multiline_continuation`
- `pass_string_quoted_keys`
- `pass_string_array_inline_mix`
- `pass_string_comment_boundary`
- `pass_string_unicode_escape`
- `pass_string_multiline_inline_table`
- `fail_string_unterminated_basic`
- `fail_string_unterminated_literal`
- `fail_string_invalid_escape`
- `fail_string_invalid_hex`
- `fail_string_invalid_u_short`
- `fail_string_invalid_U_range`
- `fail_string_invalid_u_surrogate`
- `fail_string_newline_in_basic`
- `fail_string_newline_in_literal`
- `fail_string_multiline_key`

12. Arrays / nested arrays / array-of-tables
- `pass_nested_arrays`
- `pass_array_of_tables_basic`
- `pass_array_of_tables_nested`
- `fail_array_table_redefine`
- `fail_array_of_tables_unsupported`

13. Table conflict matrix (cross-combination coverage)
- `pass_matrix_super_after_dotted`
- `pass_matrix_dotted_after_explicit_nested`
- `pass_matrix_aot_append`
- `pass_matrix_aot_nested_table`
- `pass_matrix_aot_then_scalar_context`
- `pass_matrix_table_then_scalar_context`
- `fail_matrix_scalar_then_table`
- `fail_matrix_scalar_then_dotted`
- `fail_matrix_dotted_then_scalar`
- `fail_matrix_scalar_then_aot`
- `fail_matrix_dotted_then_aot`
- `fail_matrix_inline_then_header`
- `fail_matrix_inline_then_nested_header`
- `fail_matrix_inline_then_dotted`

14. Date semantics (leap year and month/day constraints)
- `pass_date_semantics`
- `fail_date_non_leap_feb29`
- `fail_date_century_non_leap`
- `fail_date_invalid_day_30_month`
- `fail_datetime_invalid_day`

15. Key exclusion policy for generated members
- `pass_key_exclusion_rules`
- `fail_key_member_access_m_canonical`
- `fail_key_member_access_key_name`

16. Compile-time multi-key get path
- `pass_get_multi_key_path`
- `fail_get_multi_key_missing`

17. Compile-time mixed key/index path (`get<"..."_k, N_i, ...>`)
- `pass_get_mixed_ct_path`
- `fail_get_mixed_ct_bad_segment`

## Case Layout

Each case directory contains:

- `main.cpp` - `case.toml`

`test / CMakeLists.txt` discovers cases via `GLOB + CONFIGURE_DEPENDS`.
