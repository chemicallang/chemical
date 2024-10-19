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
func atof(str : *char) : double

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
func atoi(str : *char) : int

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
func atol(str : *char) : long

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
func atoll(str  :*char) : bigint

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
func strtol(str : *char, str_end : **mut char, base : int) : long

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
func strtoll(str : *char, str_end : **mut char, base : int) : bigint

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
func strtoul(str : *mut char, str_end : **mut char, base : int) : ulong

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
func strtoull(str : *mut char, str_end : **mut char, base : int) : ubigint

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
func strtof(str : *mut char, str_end : **mut char) : float

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
func strtod(str : *mut char, str_end : **mut char) : double

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
func strtold(str : *mut char, str_end : **mut char) : float128

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
func strfromf(s : *char, n : size_t, format : *char, fp : float) : int

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
func strfromd(s : *char, n : size_t, format : *char, fp : double) : int

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
func strfroml(s : *char, n : size_t, format : *char, fp : float128) : int

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
func abs(n : int) : int

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
func labs(n : long) : long

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
func llabs(n : bigint) : bigint

/**
 * the result of div function below
 */
struct div_t {
    const quot : int
    const rem : int
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
func div(x : int, y : int) : div_t

/**
 * the result of ldiv function below
 */
struct ldiv_t {
    const quot : long;
    const rem : long;
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
func ldiv(x : long, y : long) : ldiv_t


/**
 * the result of lldiv function below
 */
struct lldiv_t {
    const quot : bigint;
    const rem : bigint;
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
func lldiv(x : bigint, y : bigint) : lldiv_t

/**
 * Causes abnormal program termination unless SIGABRT is being caught by a signal handler passed to signal and the handler does not return.
 * Functions passed to atexit() are not called. Whether open resources such as files are closed is implementation defined. An implementation defined status is returned to the host environment that indicates unsuccessful execution.
 * @see https://en.cppreference.com/w/c/program/abort
 */
func abort();

/**
 * Causes normal program termination to occur.
 * Several cleanup steps are performed:
 *      functions passed to atexit are called, in reverse order of registration
 *      all C streams are flushed and closed
 *      files created by tmpfile are removed
 *      control is returned to the host environment. If exit_code is zero or EXIT_SUCCESS, an implementation-defined status indicating successful termination is returned. If exit_code is EXIT_FAILURE, an implementation-defined status indicating unsuccessful termination is returned. In other cases implementation-defined status value is returned.
 * @see https://en.cppreference.com/w/c/program/exit
 */
func exit(exit_code : int);

/**
 * Causes normal program termination to occur without completely cleaning the resources.
 * Functions passed to at_quick_exit are called in reverse order of their registration. After calling the registered functions, calls _Exit(exit_code).
 * Functions passed to atexit or signal handlers passed to signal are not called.
 * @param exit_code	-	exit status of the program
 * @see https://en.cppreference.com/w/c/program/quick_exit
 */
func quick_exit(exit_code : int)

/**
 * Causes normal program termination to occur without completely cleaning the resources.
 * Functions passed to at_quick_exit() or atexit() are not called. Whether open streams with unwritten buffered data are flushed, open streams are closed, or temporary files are removed is implementation-defined.
 * If exit_code is 0 or EXIT_SUCCESS, an implementation-defined status indicating successful termination is returned to the host environment. If exit_code is EXIT_FAILURE, an implementation-defined status, indicating unsuccessful termination, is returned. In other cases an implementation-defined status value is returned.
 * @param exit_code	-	exit status of the program
 * @see https://en.cppreference.com/w/c/program/_Exit
 */
func _Exit(exit_code : int)

/**
 * Registers the function pointed to by func to be called on normal program termination (via exit() or returning from main()). The functions will be called in reverse order they were registered, i.e. the function registered last will be executed first.
 * The same function may be registered more than once.
 * The implementation is guaranteed to support the registration of at least 32 functions. The exact limit is implementation-defined.
 * @param func	-	pointer to a function to be called on normal program termination
 * @return 0 if the registration succeeds, nonzero value otherwise.
 * @see https://en.cppreference.com/w/c/program/atexit
 */
func atexit(func : () => void ) : int

/**
 * Registers the function pointed to by func to be called on quick program termination (via quick_exit).
 * Calling the function from several threads does not induce a data race. The implementation is guaranteed to support the registration of at least 32 functions. The exact limit is implementation-defined.
 * The registered functions will not be called on normal program termination. If a function need to be called in that case, atexit must be used.
 * @param func	-	pointer to a function to be called on quick program termination
 * @return 0 if the registration succeeds, nonzero value otherwise.
 * @see https://en.cppreference.com/w/c/program/at_quick_exit
 */
func at_quick_exit(func : () => void) : int

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
func system(command : *char) : int

/**
 * Searches for an environmental variable with name name in the host-specified environment list and returns a pointer to the string that is associated with the matched environment variable. The set of environmental variables and methods of altering it are implementation-defined.
 * This function is not required to be thread-safe. Another call to getenv, as well as a call to the POSIX functions setenv(), unsetenv(), and putenv() may invalidate the pointer returned by a previous call or modify the string obtained from a previous call.
 * Modifying the string returned by getenv invokes undefined behavior.
 * @param name	-	null-terminated character string identifying the name of the environmental variable to look for
 * @return character string identifying the value of the environmental variable or null pointer if such variable is not found.
 * @see https://en.cppreference.com/w/c/program/getenv
 */
func getenv(name : *char) : *mut char

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
func getenv_s(
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
func memalignment(p : *void) : size_t