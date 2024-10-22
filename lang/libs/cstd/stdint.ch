import "common/integer_types.ch"

/**
 * TODO Some types aren't exact
 * TODO we must go over these again, to make sure these types work with functions in C and are completely compatible
 * TODO integer constants aren't done, for example limits or min max values
 * see https://en.cppreference.com/w/c/types/integer
 */


@comptime
const INT8_MIN = (-127i8 - 1)

@comptime
const INT16_MIN = (-32767i16 - 1)

@comptime
const INT32_MIN = (-2147483647i32 - 1)

@comptime
const INT64_MIN = (-9223372036854775807i64 - 1)

@comptime
const INT8_MAX = 127i8

@comptime
const INT16_MAX = 32767i16

@comptime
const INT32_MAX = 2147483647i32

@comptime
const INT64_MAX = 9223372036854775807i64

@comptime
const UINT8_MAX = 0xffui8

@comptime
const UINT16_MAX = 0xffffui16

@comptime
const UINT32_MAX = 0xffffffffu32

@comptime
const UINT64_MAX = 0xffffffffffffffffu64

@comptime
const INT_LEAST8_MIN = INT8_MIN
@comptime
const INT_LEAST16_MIN = INT16_MIN
@comptime
const INT_LEAST32_MIN = INT32_MIN
@comptime
const INT_LEAST64_MIN = INT64_MIN
@comptime
const INT_LEAST8_MAX = INT8_MAX
@comptime
const INT_LEAST16_MAX = INT16_MAX
@comptime
const INT_LEAST32_MAX = INT32_MAX
@comptime
const INT_LEAST64_MAX = INT64_MAX
@comptime
const UINT_LEAST8_MAX = UINT8_MAX
@comptime
const UINT_LEAST16_MAX = UINT16_MAX
@comptime
const UINT_LEAST32_MAX = UINT32_MAX
@comptime
const UINT_LEAST64_MAX = UINT64_MAX

@comptime
const INT_FAST8_MIN = INT8_MIN
@comptime
const INT_FAST16_MIN = INT32_MIN
@comptime
const INT_FAST32_MIN = INT32_MIN
@comptime
const INT_FAST64_MIN = INT64_MIN
@comptime
const INT_FAST8_MAX = INT8_MAX
@comptime
const INT_FAST16_MAX = INT32_MAX
@comptime
const INT_FAST32_MAX = INT32_MAX
@comptime
const INT_FAST64_MAX = INT64_MAX
@comptime
const UINT_FAST8_MAX = UINT8_MAX
@comptime
const UINT_FAST16_MAX = UINT32_MAX
@comptime
const UINT_FAST32_MAX = UINT32_MAX
@comptime
const UINT_FAST64_MAX = UINT64_MAX

if(def.win64) {
    @comptime
    const INTPTR_MIN = INT64_MIN
    @comptime
    const INTPTR_MAX = INT64_MAX
    @comptime
    const UINTPTR_MAX = UINT64_MAX
} else {
    @comptime
    const INTPTR_MIN = INT32_MIN
    @comptime
    const INTPTR_MAX = INT32_MAX
    @comptime
    const UINTPTR_MAX = UINT32_MAX
}

@comptime
const INTMAX_MIN = INT64_MIN
@comptime
const INTMAX_MAX = INT64_MAX
@comptime
const UINTMAX_MAX = UINT64_MAX

@comptime
const PTRDIFF_MIN = INTPTR_MIN
@comptime
const PTRDIFF_MAX = INTPTR_MAX

@comptime
const SIG_ATOMIC_MIN = INT32_MIN
@comptime
const SIG_ATOMIC_MAX = INT32_MAX

@comptime
const WCHAR_MIN = 0x0000
@comptime
const WCHAR_MAX = 0xffff

@comptime
const WINT_MIN = 0x0000
@comptime
const WINT_MAX = 0xffff