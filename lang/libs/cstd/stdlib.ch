import "./common/std_types.ch"
import "./common/wchar_types.ch"

/**
 * Interprets a floating-point value in a byte string pointed to by str.
 * Function discards any whitespace characters (as determined by isspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
 * decimal floating-point expression. It consists of the following parts:
 *      (optional) plus or minus sign
 *      nonempty sequence of decimal digits optionally containing decimal-point character (as determined by the current C locale) (defines significand)
 *      (optional) e or E followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 10)
 * hexadecimal floating-point expression. It consists of the following parts:
 *      (optional) plus or minus sign
 *      0x or 0X
 *      nonempty sequence of hexadecimal digits optionally containing a decimal-point character (as determined by the current C locale) (defines significand)
 *      (optional) p or P followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 2)
 * infinity expression. It consists of the following parts:
 *      (optional) plus or minus sign
 *      INF or INFINITY ignoring case
 * not-a-number expression. It consists of the following parts:
 *      (optional) plus or minus sign
 *      NAN or NAN(char_sequence) ignoring case of the NAN part. char_sequence can only contain digits, Latin letters, and underscores. The result is a quiet NaN floating-point value.
 * any other expression that may be accepted by the currently installed C locale
 * @return double value corresponding to the contents of str on success. If the converted value falls out of range of the return type, the return value is undefined. If no conversion can be performed, 0.0 is returned.
 * @see https://en.cppreference.com/w/c/string/byte/atof
 */
public func atof(str : *char) : double

/**
 * Interprets an integer value in a byte string pointed to by str. The implied radix is always 10.
 * Discards any whitespace characters until the first non-whitespace character is found, then takes as many characters as possible to form a valid integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      numeric digits
 * If the value of the result cannot be represented, i.e. the converted value falls out of range of the corresponding return type, the behavior is undefined.
 * @return Integer value corresponding to the contents of str on success.
 * If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/atoi
 */
public func atoi(str : *char) : int

/**
 * Interprets an integer value in a byte string pointed to by str. The implied radix is always 10.
 * Discards any whitespace characters until the first non-whitespace character is found, then takes as many characters as possible to form a valid integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      numeric digits
 * If the value of the result cannot be represented, i.e. the converted value falls out of range of the corresponding return type, the behavior is undefined.
 * @return Integer value corresponding to the contents of str on success.
 * If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/atoi
 */
public func atol(str : *char) : long

/**
 * Interprets an integer value in a byte string pointed to by str. The implied radix is always 10.
 * Discards any whitespace characters until the first non-whitespace character is found, then takes as many characters as possible to form a valid integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      numeric digits
 * If the value of the result cannot be represented, i.e. the converted value falls out of range of the corresponding return type, the behavior is undefined.
 * @return Integer value corresponding to the contents of str on success.
 * If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/atoi
 */
public func atoll(str  :*char) : bigint

/**
 * Interprets an integer value in a byte string pointed to by str.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions set the pointer pointed to by str_end to point to the character past the last numeric character interpreted. If str_end is a null pointer, it is ignored.
 * If the str is empty or does not have the expected form, no conversion is performed, and (if str_end is not a null pointer) the value of str is stored in the object pointed to by str_end.
 * @param str pointer to the null-terminated byte string to be interpreted, TODO str restricted
 * @param str_end pointer to a pointer to character. TODO str_end restricted
 * @param base base of the interpreted integer value
 * @return If successful, an integer value corresponding to the contents of str is returned.
 * If the converted value falls out of range of corresponding return type, a range error occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is returned.
 * If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtol
 */
public func strtol(str : *char, str_end : **mut char, base : int) : long

/**
 * Interprets an integer value in a byte string pointed to by str.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions set the pointer pointed to by str_end to point to the character past the last numeric character interpreted. If str_end is a null pointer, it is ignored.
 * If the str is empty or does not have the expected form, no conversion is performed, and (if str_end is not a null pointer) the value of str is stored in the object pointed to by str_end.
 * @param str pointer to the null-terminated byte string to be interpreted, TODO str restricted
 * @param str_end pointer to a pointer to character. TODO str_end restricted
 * @param base base of the interpreted integer value
 * @return If successful, an integer value corresponding to the contents of str is returned.
 * If the converted value falls out of range of corresponding return type, a range error occurs (setting errno to ERANGE) and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is returned.
 * If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtol
 */
public func strtoll(str : *char, str_end : **mut char, base : int) : bigint

/**
 * Interprets an unsigned integer value in a byte string pointed to by str.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) unsigned integer number representation and converts them to an integer value. The valid unsigned integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type, which applies unsigned integer wraparound rules.
 * The functions sets the pointer pointed to by str_end to point to the character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str pointer to the null-terminated byte string to be interpreted TODO str restricted
 * @param str_end pointer to a pointer to character, might be set to a position past the last character interpreted TODO str_end restricted
 * @param base base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs (errno is set to ERANGE) and ULONG_MAX or ULLONG_MAX is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtoul
 */
public func strtoul(str : *mut char, str_end : **mut char, base : int) : ulong

/**
 * Interprets an unsigned integer value in a byte string pointed to by str.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) unsigned integer number representation and converts them to an integer value. The valid unsigned integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type, which applies unsigned integer wraparound rules.
 * The functions sets the pointer pointed to by str_end to point to the character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str pointer to the null-terminated byte string to be interpreted TODO str restricted
 * @param str_end pointer to a pointer to character, might be set to a position past the last character interpreted TODO str_end restricted
 * @param base base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs (errno is set to ERANGE) and ULONG_MAX or ULLONG_MAX is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtoul
 */
public func strtoull(str : *mut char, str_end : **mut char, base : int) : ubigint

/**
 * Interprets a floating-point value in a byte string pointed to by str.
 * Function discards any whitespace characters (as determined by isspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
 *      decimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          nonempty sequence of decimal digits optionally containing decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) e or E followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 10)
 *      hexadecimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          0x or 0X
 *          nonempty sequence of hexadecimal digits optionally containing a decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) p or P followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 2)
 *      infinity expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          INF or INFINITY ignoring case
 *      not-a-number expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          NAN or NAN(char_sequence) ignoring case of the NAN part. char_sequence can only contain digits, Latin letters, and underscores. The result is a quiet NaN floating-point value.
 *      any other expression that may be accepted by the currently installed C locale
 * The functions sets the pointer pointed to by str_end to point to the character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str pointer to the null-terminated byte string to be interpreted TODO restricted
 * @param str_end pointer to a pointer to character. TODO restricted
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs (errno is set to ERANGE) and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtof
 */
public func strtof(str : *mut char, str_end : **mut char) : float

/**
 * Interprets a floating-point value in a byte string pointed to by str.
 * Function discards any whitespace characters (as determined by isspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
 *      decimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          nonempty sequence of decimal digits optionally containing decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) e or E followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 10)
 *      hexadecimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          0x or 0X
 *          nonempty sequence of hexadecimal digits optionally containing a decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) p or P followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 2)
 *      infinity expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          INF or INFINITY ignoring case
 *      not-a-number expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          NAN or NAN(char_sequence) ignoring case of the NAN part. char_sequence can only contain digits, Latin letters, and underscores. The result is a quiet NaN floating-point value.
 *      any other expression that may be accepted by the currently installed C locale
 * The functions sets the pointer pointed to by str_end to point to the character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str pointer to the null-terminated byte string to be interpreted TODO restricted
 * @param str_end pointer to a pointer to character. TODO restricted
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs (errno is set to ERANGE) and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtof
 */
public func strtod(str : *mut char, str_end : **mut char) : double

/**
 * Interprets a floating-point value in a byte string pointed to by str.
 * Function discards any whitespace characters (as determined by isspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
 *      decimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          nonempty sequence of decimal digits optionally containing decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) e or E followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 10)
 *      hexadecimal floating-point expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          0x or 0X
 *          nonempty sequence of hexadecimal digits optionally containing a decimal-point character (as determined by the current C locale) (defines significand)
 *          (optional) p or P followed with optional minus or plus sign and nonempty sequence of decimal digits (defines exponent to base 2)
 *      infinity expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          INF or INFINITY ignoring case
 *      not-a-number expression. It consists of the following parts:
 *          (optional) plus or minus sign
 *          NAN or NAN(char_sequence) ignoring case of the NAN part. char_sequence can only contain digits, Latin letters, and underscores. The result is a quiet NaN floating-point value.
 *      any other expression that may be accepted by the currently installed C locale
 * The functions sets the pointer pointed to by str_end to point to the character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str pointer to the null-terminated byte string to be interpreted TODO restricted
 * @param str_end pointer to a pointer to character. TODO restricted
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs (errno is set to ERANGE) and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtof
 */
public func strtold(str : *mut char, str_end : **mut char) : longdouble

/**
 * Converts a floating-point value to a byte string.
 * The functions are equivalent to snprintf(s, n, format, fp), except that the format string shall only contain the character %, an optional precision that does not contain an asterisk *, and one of the conversion specifiers a, A, e, E, f, F, g, or G, which applies to the type double, float, or long double) indicated by the function suffix (rather than by a length modifier). Use of these functions with any other format string results in undefined behavior.
 * @param s	-	pointer to a character string to write to TODO restricted
 * @param n	-	up to n - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data  TODO restricted
 * @param fp	-	floating-point value to convert
 * @return The number of characters that would have been written had n been sufficiently large, not counting the terminating null character. Thus, the null-terminated output has been completely written if and only if the returned value is both nonnegative and less than n.
 * @see https://en.cppreference.com/w/c/string/byte/strfromf
 */
public func strfromf(s : *char, n : size_t, format : *char, fp : float) : int

/**
 * Converts a floating-point value to a byte string.
 * The functions are equivalent to snprintf(s, n, format, fp), except that the format string shall only contain the character %, an optional precision that does not contain an asterisk *, and one of the conversion specifiers a, A, e, E, f, F, g, or G, which applies to the type double, float, or long double) indicated by the function suffix (rather than by a length modifier). Use of these functions with any other format string results in undefined behavior.
 * @param s	-	pointer to a character string to write to TODO restricted
 * @param n	-	up to n - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data  TODO restricted
 * @param fp	-	floating-point value to convert
 * @return The number of characters that would have been written had n been sufficiently large, not counting the terminating null character. Thus, the null-terminated output has been completely written if and only if the returned value is both nonnegative and less than n.
 * @see https://en.cppreference.com/w/c/string/byte/strfromf
 */
public func strfromd(s : *char, n : size_t, format : *char, fp : double) : int

/**
 * Converts a floating-point value to a byte string.
 * The functions are equivalent to snprintf(s, n, format, fp), except that the format string shall only contain the character %, an optional precision that does not contain an asterisk *, and one of the conversion specifiers a, A, e, E, f, F, g, or G, which applies to the type double, float, or long double) indicated by the function suffix (rather than by a length modifier). Use of these functions with any other format string results in undefined behavior.
 * @param s	-	pointer to a character string to write to TODO restricted
 * @param n	-	up to n - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data  TODO restricted
 * @param fp	-	floating-point value to convert
 * @return The number of characters that would have been written had n been sufficiently large, not counting the terminating null character. Thus, the null-terminated output has been completely written if and only if the returned value is both nonnegative and less than n.
 * @see https://en.cppreference.com/w/c/string/byte/strfromf
 */
public func strfroml(s : *char, n : size_t, format : *char, fp : longdouble) : int

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
public func abs(n : int) : int

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
public func labs(n : long) : long

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
public func llabs(n : bigint) : bigint

/**
 * the result of div function below
 */
public struct div_t {
    const quot : int
    const rem : int
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
public func div(x : int, y : int) : div_t

/**
 * the result of ldiv function below
 */
public struct ldiv_t {
    const quot : long;
    const rem : long;
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
public func ldiv(x : long, y : long) : ldiv_t


/**
 * the result of lldiv function below
 */
public struct lldiv_t {
    const quot : bigint;
    const rem : bigint;
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
public func lldiv(x : bigint, y : bigint) : lldiv_t

/**
 * Causes abnormal program termination unless SIGABRT is being caught by a signal handler passed to signal and the handler does not return.
 * Functions passed to atexit() are not called. Whether open resources such as files are closed is implementation defined. An implementation defined status is returned to the host environment that indicates unsuccessful execution.
 * @see https://en.cppreference.com/w/c/program/abort
 */
@no_return
public func abort();

/**
 * Causes normal program termination to occur.
 * Several cleanup steps are performed:
 *      functions passed to atexit are called, in reverse order of registration
 *      all C streams are flushed and closed
 *      files created by tmpfile are removed
 *      control is returned to the host environment. If exit_code is zero or EXIT_SUCCESS, an implementation-defined status indicating successful termination is returned. If exit_code is EXIT_FAILURE, an implementation-defined status indicating unsuccessful termination is returned. In other cases implementation-defined status value is returned.
 * @see https://en.cppreference.com/w/c/program/exit
 */
@no_return
public func exit(exit_code : int);

/**
 * Causes normal program termination to occur without completely cleaning the resources.
 * Functions passed to at_quick_exit are called in reverse order of their registration. After calling the registered functions, calls _Exit(exit_code).
 * Functions passed to atexit or signal handlers passed to signal are not called.
 * @param exit_code	-	exit status of the program
 * @see https://en.cppreference.com/w/c/program/quick_exit
 */
@no_return
public func quick_exit(exit_code : int)

/**
 * Causes normal program termination to occur without completely cleaning the resources.
 * Functions passed to at_quick_exit() or atexit() are not called. Whether open streams with unwritten buffered data are flushed, open streams are closed, or temporary files are removed is implementation-defined.
 * If exit_code is 0 or EXIT_SUCCESS, an implementation-defined status indicating successful termination is returned to the host environment. If exit_code is EXIT_FAILURE, an implementation-defined status, indicating unsuccessful termination, is returned. In other cases an implementation-defined status value is returned.
 * @param exit_code	-	exit status of the program
 * @see https://en.cppreference.com/w/c/program/_Exit
 */
@no_return
public func _Exit(exit_code : int)

/**
 * Registers the function pointed to by func to be called on normal program termination (via exit() or returning from main()). The functions will be called in reverse order they were registered, i.e. the function registered last will be executed first.
 * The same function may be registered more than once.
 * The implementation is guaranteed to support the registration of at least 32 functions. The exact limit is implementation-defined.
 * @param func	-	pointer to a function to be called on normal program termination
 * @return 0 if the registration succeeds, nonzero value otherwise.
 * @see https://en.cppreference.com/w/c/program/atexit
 */
public func atexit(func : () => void ) : int

/**
 * Registers the function pointed to by func to be called on quick program termination (via quick_exit).
 * Calling the function from several threads does not induce a data race. The implementation is guaranteed to support the registration of at least 32 functions. The exact limit is implementation-defined.
 * The registered functions will not be called on normal program termination. If a function need to be called in that case, atexit must be used.
 * @param func	-	pointer to a function to be called on quick program termination
 * @return 0 if the registration succeeds, nonzero value otherwise.
 * @see https://en.cppreference.com/w/c/program/at_quick_exit
 */
public func at_quick_exit(func : () => void) : int

/**
 * TODO these macros haven't been defined
 * #define EXIT_SUCCESS // implementation defined
 * #define EXIT_FAILURE // implementation defined
 * @see https://en.cppreference.com/w/c/program/EXIT_status
 */

/**
 * Calls the host environment's command processor with the parameter command. Returns an implementation-defined value (usually the value that the invoked program returns).
 * If command is a null pointer, checks if the host environment has a command processor and returns a nonzero value if and only if the command processor exists.
 * @param command	-	character string identifying the command to be run in the command processor. If a null pointer is given, command processor is checked for existence
 * @return Implementation-defined value. If command is a null pointer, returns a nonzero value if and only if the command processor exists.
 * @see https://en.cppreference.com/w/c/program/system
 */
public func system(command : *char) : int

/**
 * Searches for an environmental variable with name name in the host-specified environment list and returns a pointer to the string that is associated with the matched environment variable. The set of environmental variables and methods of altering it are implementation-defined.
 * This function is not required to be thread-safe. Another call to getenv, as well as a call to the POSIX functions setenv(), unsetenv(), and putenv() may invalidate the pointer returned by a previous call or modify the string obtained from a previous call.
 * Modifying the string returned by getenv invokes undefined behavior.
 * @param name	-	null-terminated character string identifying the name of the environmental variable to look for
 * @return character string identifying the value of the environmental variable or null pointer if such variable is not found.
 * @see https://en.cppreference.com/w/c/program/getenv
 */
public func getenv(name : *char) : *mut char

/**
 * Same as (1), except that the values of the environment variable is written to the user-provided buffer value (unless null) and the number of bytes written is stored in the user-provided location *len (unless null). If the environment variable is not set in the environment, zero is written to *len (unless null) and '\0' is written to value[0] (unless null). In addition, the following errors are detected at runtime and call the currently installed constraint handler function:
 * name is a null pointer
 * valuesz is greater than RSIZE_MAX
 * value is a null pointer and valuesz is not zero
 * As with all bounds-checked functions, getenv_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdlib.h>.
 * @param name	-	null-terminated character string identifying the name of the environmental variable to look for TODO restricted
 * @param len	-	pointer to a user-provided location where getenv_s will store the length of the environment variable TODO restricted
 * @param value	-	pointer to a user-provided character array where getenv_s will store the contents of the environment variable TODO restricted
 * @param valuesz	-	maximum number of characters that getenv_s is allowed to write to dest (size of the buffer)
 * @return zero if the environment variable was found, non-zero if it was not found or if a runtime constraint violation occurred. On any error, writes zero to *len (unless len is a null pointer).
 * @see https://en.cppreference.com/w/c/program/getenv
 */
public func getenv_s(
    len : *mut size_t,
    value : *mut char,
    valuesz : rsize_t,
    name : *char
) : errno_t

/**
 * Returns the maximum alignment satisfied by the provided address. The return value can be greater than any alignment value supported by the implementation. If p is a null pointer value, ​0​ is returned to indicate that the pointer cannot be used to access an object of any type.
 * If the return value compares is greater than or equal to alignof(T), the alignment requirement for the type T is satisfied by the pointer.
 * A freestanding implementation needs to provide memalignment.
 * @param p	-	pointer to query alignment
 * @return The alignment value of p, or 0 if p is a null pointer value.
 * @see https://en.cppreference.com/w/c/program/memalignment
 */
public func memalignment(p : *void) : size_t

/**
 * Determines the size, in bytes, of the multibyte character whose first byte is pointed to by s.
 * If s is a null pointer, resets the global conversion state and(until C23) determined whether shift sequences are used.
 * This function is equivalent to the call mbtowc((wchar_t*)0, s, n), except that conversion state of mbtowc is unaffected.
 * @param s	-	pointer to the multibyte character
 * @param n	-	limit on the number of bytes in s that can be examined
 * @return If s is not a null pointer, returns the number of bytes that are contained in the multibyte character or -1 if the first bytes pointed to by s do not form a valid multibyte character or ​0​ if s is pointing at the null charcter '\0'.
 * If s is a null pointer, resets its internal conversion state to represent the initial shift state and(until C23) returns ​0​ if the current multibyte encoding is not state-dependent (does not use shift sequences) or a non-zero value if the current multibyte encoding is state-dependent (uses shift sequences).
 * @see https://en.cppreference.com/w/c/string/multibyte/mblen
 */
public func mblen(s : *char, n : size_t) : int

/**
 * Converts a multibyte character whose first byte is pointed to by s to a wide character, written to *pwc if pwc is not null.
 * If s is a null pointer, resets the global conversion state and determines whether shift sequences are used.
 * @param pwc	-	pointer to the wide character for output
 * @param s	-	pointer to the multibyte character
 * @param n	-	limit on the number of bytes in s that can be examined
 * @return If s is not a null pointer, returns the number of bytes that are contained in the multibyte character or -1 if the first bytes pointed to by s do not form a valid multibyte character or ​0​ if s is pointing at the null character '\0'.
 *         If s is a null pointer, resets its internal conversion state to represent the initial shift state and returns ​0​ if the current multibyte encoding is not state-dependent (does not use shift sequences) or a non-zero value if the current multibyte encoding is state-dependent (uses shift sequences).
 * @see https://en.cppreference.com/w/c/string/multibyte/mbtowc
 */
public func mbtowc(pwc : *mut wchar_t, s : *char, n : size_t) : int

/**
 * Converts a wide character wc to multibyte encoding and stores it (including any shift sequences) in the char array whose first element is pointed to by s. No more than MB_CUR_MAX characters are stored. The conversion is affected by the current locale's LC_CTYPE category.
 * If wc is the null character, the null byte is written to s, preceded by any shift sequences necessary to restore the initial shift state.
 * If s is a null pointer, this function resets the global conversion state and determines whether shift sequences are used.
 * @param s	-	pointer to the character array for output
 * @param wc	-	wide character to convert
 * @param ssz	-	maximum number of bytes to write to s (size of the array s)
 * @param status	-	pointer to an out-parameter where the result (length of the multibyte sequence or the shift sequence status) will be stored
 * @return If s is not a null pointer, returns the number of bytes that are contained in the multibyte representation of wc or -1 if wc is not a valid character.
 *         If s is a null pointer, resets its internal conversion state to represent the initial shift state and returns ​0​ if the current multibyte encoding is not state-dependent (does not use shift sequences) or a non-zero value if the current multibyte encoding is state-dependent (uses shift sequences).
 * @see https://en.cppreference.com/w/c/string/multibyte/wctomb
 */
public func wctomb(s : *char, wc : wchar_t) : int

/**
 * Same as (1), except that the result is returned in the out-parameter status and the following errors are detected at runtime and call the currently installed constraint handler function:
 *      ssz is less than the number of bytes that would be written (unless s is null)
 *      ssz is greater than RSIZE_MAX (unless s is null)
 *      s is a null pointer but ssz is not zero
 *  As with all bounds-checked functions, wctomb_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdlib.h>.
 * @param s	-	pointer to the character array for output
 * @param wc	-	wide character to convert
 * @param ssz	-	maximum number of bytes to write to s (size of the array s)
 * @param status	-	pointer to an out-parameter where the result (length of the multibyte sequence or the shift sequence status) will be stored
 * @return zero on success, in which case the multibyte representation of wc is stored in s and its length is stored in *status, or, if s is null, the shift sequence status is stored in status). Non-zero on encoding error or runtime constraint violation, in which case (size_t)-1 is stored in *status. The value stored in *status never exceeds MB_CUR_MAX
 * @see https://en.cppreference.com/w/c/string/multibyte/wctomb
 */
public func wctomb_s(status : *mut int, s : *mut char, ssz : rsize_t, wc : wchar_t) : errno_t

/**
 * Converts a multibyte character string from the array whose first element is pointed to by src to its wide character representation. Converted characters are stored in the successive elements of the array pointed to by dst. No more than len wide characters are written to the destination array.
 *  Each character is converted as if by a call to mbtowc, except that the mbtowc conversion state is unaffected. The conversion stops if:
 *   * The multibyte null character was converted and stored.
 *   * An invalid (in the current C locale) multibyte character was encountered.
 *   * The next wide character to be stored would exceed len.
 *  If src and dst overlap, the behavior is undefined
 * @param dst	-	pointer to wide character array where the wide string will be stored
 * @param src	-	pointer to the first element of a null-terminated multibyte string to convert
 * @param len	-	number of wide characters available in the array pointed to by dst
 * @param dstsz	-	max number of wide characters that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return On success, returns the number of wide characters, excluding the terminating L'\0', written to the destination array. On conversion error (if invalid multibyte character was encountered), returns (size_t)-1.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbstowcs
 */
public func mbstowcs(dst : *mut wchar_t, src : *char, len : size_t) : size_t

/**
 *  Same as (1), except that
 *   * conversion is as-if by mbrtowc, not mbtowc
 *   * the function returns its result as an out-parameter retval
 *   * if no null character was written to dst after len wide characters were written, then L'\0' is stored in dst[len], which means len+1 total wide characters are written
 *   * if dst is a null pointer, the number of wide characters that would be produced is stored in *retval
 *   * the function clobbers the destination array from the terminating null and until dstsz
 *   * If src and dst overlap, the behavior is unspecified.
 *   * the following errors are detected at runtime and call the currently installed constraint handler function:
 *          retval or src is a null pointer
 *          dstsz or len is greater than RSIZE_MAX/sizeof(wchar_t) (unless dst is null)
 *          dstsz is not zero (unless dst is null)
 *          There is no null character in the first dstsz multibyte characters in the src array and len is greater than dstsz (unless dst is null)
 *  As with all bounds-checked functions, mbstowcs_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdlib.h>.
 * @param dst	-	pointer to wide character array where the wide string will be stored
 * @param src	-	pointer to the first element of a null-terminated multibyte string to convert
 * @param len	-	number of wide characters available in the array pointed to by dst
 * @param dstsz	-	max number of wide characters that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return zero on success (in which case the number of wide characters excluding terminating zero that were, or would be written to dst, is stored in *retval), non-zero on error. In case of a runtime constraint violation, stores (size_t)-1 in *retval (unless retval is null) and sets dst[0] to L'\0' (unless dst is null or dstmax is zero or greater than RSIZE_MAX)
 * @see https://en.cppreference.com/w/c/string/multibyte/mbstowcs
 */
public func mbstowcs_s(retval : *mut size_t, dst : *mut wchar_t, dstsz : rsize_t, src : *char, len : rsize_t) : errno_t

/**
 * Converts a sequence of wide characters from the array whose first element is pointed to by src to its narrow multibyte representation that begins in the initial shift state. Converted characters are stored in the successive elements of the char array pointed to by dst. No more than len bytes are written to the destination array.
 *  Each character is converted as if by a call to wctomb, except that the wctomb's conversion state is unaffected. The conversion stops if:
 *   * The null character L'\0' was converted and stored. The bytes stored in this case are the unshift sequence (if necessary) followed by '\0',
 *   * A wchar_t was found that does not correspond to a valid character in the current C locale.
 *   * The next multibyte character to be stored would exceed len.
 *  If src and dst overlap, the behavior is unspecified.
 * @param dst	-	pointer to narrow character array where the multibyte character will be stored
 * @param src	-	pointer to the first element of a null-terminated wide string to convert
 * @param len	-	number of bytes available in the array pointed to by dst
 * @param dstsz	-	max number of bytes that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return On success, returns the number of bytes (including any shift sequences, but excluding the terminating '\0') written to the character array whose first element is pointed to by dst. On conversion error (if invalid wide character was encountered), returns (size_t)-1.
 * @see https://en.cppreference.com/w/c/string/multibyte/wcstombs
 */
public func wcstombs(dst : *mut char, src : *wchar_t, len : size_t) : size_t

/**
 * Same as wcstombs, except that
 *   * conversion is as-if by wcrtomb, not wctomb
 *   * the function returns its result as an out-parameter retval
 *   * if the conversion stops without writing a null character, the function will store '\0' in the next byte in dst, which may be dst[len] or dst[dstsz], whichever comes first (meaning up to len+1/dstsz+1 total bytes may be written). In this case, there may be no unshift sequence written before the terminating null.
 *   * if dst is a null pointer, the number of bytes that would be produced is stored in *retval
 *   * the function clobbers the destination array from the terminating null and until dstsz
 *   * If src and dst overlap, the behavior is unspecified.
 *   * the following errors are detected at runtime and call the currently installed constraint handler function:
 *      retval or src is a null pointer
 *      dstsz or len is greater than RSIZE_MAX (unless dst is null)
 *      dstsz is not zero (unless dst is null)
 *      len is greater than dstsz and the conversion does not encounter null or encoding error in the src array by the time dstsz is reached (unless dst is null)
 *  As with all bounds-checked functions, wcstombs_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdlib.h>.
 * @param dst	-	pointer to narrow character array where the multibyte character will be stored
 * @param src	-	pointer to the first element of a null-terminated wide string to convert
 * @param len	-	number of bytes available in the array pointed to by dst
 * @param dstsz	-	max number of bytes that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return Returns zero on success (in which case the number of bytes excluding terminating zero that were, or would be written to dst, is stored in *retval), non-zero on error. In case of a runtime constraint violation, stores (size_t)-1 in *retval (unless retval is null) and sets dst[0] to '\0' (unless dst is null or dstmax is zero or greater than RSIZE_MAX)
 * @see https://en.cppreference.com/w/c/string/multibyte/wcstombs
 */
public func wcstombs_s(retval : *size_t, dst : *char, dstsz : rsize_t, src : *wchar_t, len : rsize_t);

/**
 * TODO macro MB_CUR_MAX
 * maximum number of bytes in a multibyte character, in the current locale
 * @see https://en.cppreference.com/w/c/string/multibyte
 */

/**
 * Returns a pseudo-random integer value between 0 and RAND_MAX (0 and RAND_MAX included).
 * srand() seeds the pseudo-random number generator used by rand(). If rand() is used before any calls to srand(), rand() behaves as if it was seeded with srand(1). Each time rand() is seeded with srand(), it must produce the same sequence of values.
 * rand() is not guaranteed to be thread-safe.
 * @return Pseudo-random integer value between 0 and RAND_MAX, inclusive.
 * @note There are no guarantees as to the quality of the random sequence produced. In the past, some implementations of rand() have had serious shortcomings in the randomness, distribution and period of the sequence produced (in one well-known example, the low-order bit simply alternated between 1 and 0 between calls). rand() is not recommended for serious random-number generation needs, like cryptography.
 */
public func rand() : int

/**
 * Seeds the pseudo-random number generator used by rand() with the value seed.
 * If rand() is used before any calls to srand(), rand() behaves as if it was seeded with srand(1).
 * Each time rand() is seeded with the same seed, it must produce the same sequence of values.
 * srand() is not guaranteed to be thread-safe.
 * @param seed	-	the seed value
 * @note Generally speaking, the pseudo-random number generator should only be seeded once, before any calls to rand(), and the start of the program. It should not be repeatedly seeded, or reseeded every time you wish to generate a new batch of pseudo-random numbers.
 *       Standard practice is to use the result of a call to time(0) as the seed. However, time() returns a time_t value, and time_t is not guaranteed to be an integral type. In practice, though, every major implementation defines time_t to be an integral type, and this is also what POSIX requires.
 */
public func srand(seed : uint)