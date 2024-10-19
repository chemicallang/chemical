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