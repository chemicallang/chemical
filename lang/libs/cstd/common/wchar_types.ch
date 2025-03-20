/**
 * integer type that can hold any valid wide character and at least one more value
 * @see https://en.cppreference.com/w/cpp/header/cwchar
 */
if(def.windows) {
    @extern
public type wint_t = ushort;
} else {
    @extern
public type wint_t = int;
}

/**
 * type for wide character representation (see wide strings). It has the same size, signedness, and alignment as one of the integer types, but is a distinct type. In practice, it is 32 bits and holds UTF-32 on Linux and many other non-Windows systems, but 16 bits and holds UTF-16 code units on Windows. The standard used to require wchar_t to be large enough to represent any supported character code point. However, such requirement cannot be fulfilled on Windows, and thus it is considered as a defect and removed.
 * @see https://en.cppreference.com/w/cpp/language/types
 */
if(def.windows) {
    @extern
public type wchar_t = ushort;
} else {
    @extern
public type wchar_t = int;
}