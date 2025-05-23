// TODO https://en.cppreference.com/w/c/types/integer
//   a lot of macros haven't been done

/**
 * Interprets an integer value in a byte string pointed to by nptr.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions sets the pointer pointed to by endptr to point to the character past the last character interpreted. If endptr is a null pointer, it is ignored.
 * If the nptr is empty or does not have the expected form, no conversion is performed, and (if endptr is not a null pointer) the value of nptr is stored in the object pointed to by endptr.
 * @param nptr	-	pointer to the null-terminated byte string to be interpreted TODO restricted
 * @param endptr	-	pointer to a pointer to character. TODO restricted
 * @param base	-	base of the interpreted integer value
 * @return If successful, an integer value corresponding to the contents of str is returned.
 *         If the converted value falls out of range of corresponding return type, a range error occurs (setting errno to ERANGE) and INTMAX_MAX, INTMAX_MIN, UINTMAX_MAX or ​0​ is returned, as appropriate.
 *         If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtoimax
 */
@extern
public func strtoimax(nptr : *char, endptr : **mut char, base : int) : intmax_t

/**
 * Interprets an integer value in a byte string pointed to by nptr.
 * Discards any whitespace characters (as identified by calling isspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions sets the pointer pointed to by endptr to point to the character past the last character interpreted. If endptr is a null pointer, it is ignored.
 * If the nptr is empty or does not have the expected form, no conversion is performed, and (if endptr is not a null pointer) the value of nptr is stored in the object pointed to by endptr.
 * @param nptr	-	pointer to the null-terminated byte string to be interpreted TODO restricted
 * @param endptr	-	pointer to a pointer to character. TODO restricted
 * @param base	-	base of the interpreted integer value
 * @return If successful, an integer value corresponding to the contents of str is returned.
 *         If the converted value falls out of range of corresponding return type, a range error occurs (setting errno to ERANGE) and INTMAX_MAX, INTMAX_MIN, UINTMAX_MAX or ​0​ is returned, as appropriate.
 *         If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strtoimax
 */
@extern
public func strtoumax(nptr : *char, endptr : **mut char, base : int) : uintmax_t

/**
 * Computes the absolute value of an integer number. The behavior is undefined if the result cannot be represented by the return type.
 * @param n	-	integer value
 * @return The absolute value of n (i.e. |n|), if it is representable.
 * @see https://en.cppreference.com/w/c/numeric/math/abs
 */
@extern
public func imaxabs(n : intmax_t) : intmax_t

/**
 * the result of imaxdiv function below
 */
@extern
public struct imaxdiv_t {
    const quot : intmax_t;
    const rem : intmax_t;
};

/**
 * Computes both the quotient and the remainder of the division of the numerator x by the denominator y.
 * @param x, y	-	integer values
 * @return If both the remainder and the quotient can be represented as objects of the corresponding type (int, long, long long, intmax_t, respectively), returns both as an object of type div_t, ldiv_t, lldiv_t, imaxdiv_t
 * @see https://en.cppreference.com/w/c/numeric/math/div
 */
@extern
public func imaxdiv(x : intmax_t, y : intmax_t) : imaxdiv_t