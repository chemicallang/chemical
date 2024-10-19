/**
 * This file contains some chemical related stuff that is needed
 * to support the headers present in cstd
 * No chemical related stuff that is unrelated to C standard will ever be
 * added to cstd, this just provides support
 * Anything can be added that is comptime, which helps in provide better
 * support for cstd, This includes comptime functions that correspond directly
 * to macros or type aliases
 */

/**
 * our longdouble is always 128 bits, however long double
 * might contain different implementation, which change based on platform
 * so this is just to account for that
 */
typealias longdouble = longdouble