/**
 * TODO Some types aren't exact
 * TODO we must go over these again, to make sure these types work with functions in C and are completely compatible
 * TODO integer constants aren't done, for example limits or min max values
 * see https://en.cppreference.com/w/c/types/integer
 */

/**
 * signed integer type with width of
 * exactly 8 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int8_t = char

/**
 * signed integer type with width of
 * exactly 16 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int16_t = short

/**
 * signed integer type with width of
 * exactly 32 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int32_t = int

/**
 * signed integer type with width of
 * exactly 64 bits respectively
 * with no padding bits and using 2's complement for negative values
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int64_t = bigint

/**
 * fastest signed integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_fast8_t = char

/**
 * fastest signed integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_fast16_t = short

/**
 * fastest signed integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_fast32_t = int

/**
 * fastest signed integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_fast64_t = bigint

/**
 * smallest signed integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_least8_t = char

/**
 * smallest signed integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_least16_t = short

/**
 * smallest signed integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_least32_t = int

/**
 * smallest signed integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias int_least64_t = bigint

/**
 * maximum width integer type
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias intmax_t = bigint

/**
 * integer type capable of holding a pointer
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias intptr_t = bigint

/**
 * unsigned integer type with width of
 * exactly 8 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint8_t = uchar

/**
 * unsigned integer type with width of
 * exactly 16 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint16_t = ushort

/**
 * unsigned integer type with width of
 * exactly 32 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint32_t = uint

/**
 * unsigned integer type with width of
 * exactly 64 bits respectively
 * (provided only if the implementation directly supports the type)
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint64_t = ubigint

/**
 * fastest unsigned integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_fast8_t = uchar

/**
 * fastest unsigned integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_fast16_t = ushort

/**
 * fastest unsigned integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_fast32_t = uint

/**
 * fastest unsigned integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_fast64_t = ubigint

/**
 * smallest unsigned integer type with width of
 * at least 8 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_least8_t = uchar

/**
 * smallest unsigned integer type with width of
 * at least 16 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_least16_t = ushort

/**
 * smallest unsigned integer type with width of
 * at least 32 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_least32_t = uint

/**
 * smallest unsigned integer type with width of
 * at least 64 bits respectively
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uint_least64_t = ubigint

/**
 * maximum width unsigned integer type
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uintmax_t = ubigint

/**
 * unsigned integer type capable of holding a pointer
 * @see https://en.cppreference.com/w/c/types/integer
 */
typealias uintptr_t = ubigint