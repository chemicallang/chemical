// Copyright (c) Chemical Language Foundation 2026.

// -------------------------------------------------------
// Regression tests: fixed-size char array string literals
//
// A string literal that fits a fixed char array exactly (no null terminator)
// must be allowed. Previously the compiler used `>` instead of `>=` for the
// fit check, rejecting exact-fit literals like `char[16] = "0123456789abcdef"`
// and, in DEBUG builds, crashing via CHEM_THROW_RUNTIME("unknown").
// -------------------------------------------------------

func string_literal_exact_fit_char_array() : bool {
    const hex_lower : char[16] = "0123456789abcdef"
    if(hex_lower[0] != '0') return false
    if(hex_lower[15] != 'f') return false
    return true
}

func string_literal_exact_fit_uppercase() : bool {
    const hex_upper : char[16] = "0123456789ABCDEF"
    return hex_upper[0] == '0' && hex_upper[15] == 'F'
}

func string_literal_smaller_than_char_array() : bool {
    const s : char[8] = "abc"
    return s[0] == 'a' && s[2] == 'c'
}

func char_array_var_exact_fit() : bool {
    var buf : char[4] = "okay"
    return buf[0] == 'o' && buf[3] == 'y'
}
