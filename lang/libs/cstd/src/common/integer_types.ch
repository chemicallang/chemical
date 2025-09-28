/**
 * signed integer type with width of
 * exactly 8 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int8_t = i8

/**
 * signed integer type with width of
 * exactly 16 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int16_t = i16

/**
 * signed integer type with width of
 * exactly 32 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int32_t = i32

/**
 * signed integer type with width of
 * exactly 64 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int64_t = i64

/**
 * fastest signed integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type int_fast8_t = char
} else {
    @extern
    public type int_fast8_t = i8 // signed char
}

/**
 * fastest signed integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type int_fast16_t = short
} else {
    @extern
    public type int_fast16_t = long
}

/**
 * fastest signed integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type int_fast32_t = int
} else {
    @extern
    public type int_fast32_t = long
}

/**
 * fastest signed integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type int_fast64_t = bigint
} else {
    @extern
    public type int_fast64_t = long
}

/**
 * smallest signed integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int_least8_t = i8

/**
 * smallest signed integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int_least16_t = short

/**
 * smallest signed integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int_least32_t = int

/**
 * smallest signed integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type int_least64_t = bigint

/**
 * maximum width integer type
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type intmax_t = bigint

/**
 * integer type capable of holding a pointer
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type intptr_t = bigint
} else {
    @extern
    public type intptr_t = long
}

/**
 * unsigned integer type with width of
 * exactly 8 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint8_t = u8

/**
 * unsigned integer type with width of
 * exactly 16 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint16_t = u16

/**
 * unsigned integer type with width of
 * exactly 32 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint32_t = u32

/**
 * unsigned integer type with width of
 * exactly 64 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint64_t = u64

/**
 * fastest unsigned integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint_fast8_t = uchar

/**
 * fastest unsigned integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type uint_fast16_t = ushort
} else {
    @extern
    public type uint_fast16_t = ulong
}

/**
 * fastest unsigned integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type uint_fast32_t = uint
} else {
    @extern
    public type uint_fast32_t = ulong
}

/**
 * fastest unsigned integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type uint_fast64_t = ubigint
} else {
    @extern
    public type uint_fast64_t = ulong
}

/**
 * smallest unsigned integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint_least8_t = uchar

/**
 * smallest unsigned integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint_least16_t = ushort

/**
 * smallest unsigned integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint_least32_t = uint

/**
 * smallest unsigned integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uint_least64_t = ubigint

/**
 * maximum width unsigned integer type
 * @see https://en.cppreference.com/w/c/types/integer
 */
@extern
public type uintmax_t = ubigint

/**
 * unsigned integer type capable of holding a pointer
 * @see https://en.cppreference.com/w/c/types/integer
 */
if(def.windows) {
    @extern
    public type uintptr_t = ubigint
} else {
    @extern
    public type uintptr_t = ulong
}