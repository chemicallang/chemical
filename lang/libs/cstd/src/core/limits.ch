// TODO macros exist in this file that haven't been completed

/**
 * Number of bits in a char object (byte)	8 or greater*
 */
@comptime
public const CHAR_BIT = 8;

/**
 * Minimum value for an object of type signed char	-127 (-27+1) or less*
 */
@comptime
public const SCHAR_MIN = -128;

/**
 * Maximum value for an object of type signed char	127 (27-1) or greater*
 */
@comptime
public const SCHAR_MAX = 127;

/**
 * Maximum value for an object of type unsigned char	255 (28-1) or greater*
 */
@comptime
public const UCHAR_MAX = 0xff;

/**
 * Minimum value for an object of type char	either SCHAR_MIN or 0
 */
@comptime
public const CHAR_MIN = SCHAR_MIN;

/**
 * Maximum value for an object of type char	either SCHAR_MAX or UCHAR_MAX
 */
@comptime
public const CHAR_MAX = SCHAR_MAX;

/**
 * Maximum number of bytes in a multibyte character, for any locale	1 or greater*
 */
@comptime
public const MB_LEN_MAX = 5;

/**
 * Minimum value for an object of type short int	-32767 (-215+1) or less*
 */
@comptime
public const SHRT_MIN = -32768;

/**
 * Maximum value for an object of type short int	32767 (215-1) or greater*
 */
@comptime
public const SHRT_MAX = 32767;

/**
 * Maximum value for an object of type unsigned short int	65535 (216-1) or greater*
 */
@comptime
public const USHRT_MAX = 0xffff;

/**
 * Minimum value for an object of type int	-32767 (-215+1) or less*
 */
@comptime
public const INT_MIN = -2147483647i32 - 1;

/**
 * Maximum value for an object of type int	32767 (215-1) or greater*
 */
@comptime
public const INT_MAX = 2147483647i32;

/**
 * Maximum value for an object of type unsigned int	65535 (216-1) or greater*
 */
@comptime
public const UINT_MAX = 0xffffffffu32;

/**
 * Minimum value for an object of type long int	-2147483647 (-231+1) or less*
 */
@comptime
public const LONG_MIN = -2147483647L - 1;

/**
 * Maximum value for an object of type long int	2147483647 (231-1) or greater*
 */
@comptime
public const LONG_MAX = 2147483647L;

/**
 * Maximum value for an object of type unsigned long int	4294967295 (232-1) or greater*
 */
@comptime
public const ULONG_MAX = 0xffffffffUL;

/**
 * Minimum value for an object of type long long int	-9223372036854775807 (-263+1) or less*
 */
@comptime
public const LLONG_MIN = -9223372036854775807i64 - 1;

/**
 * Maximum value for an object of type long long int	9223372036854775807 (263-1) or greater*
 */
@comptime
public const LLONG_MAX = 9223372036854775807i64;

/**
 * Maximum value for an object of type unsigned long long int	18446744073709551615 (264-1) or greater*
 */
@comptime
public const ULLONG_MAX = 0xffffffffffffffffu64;