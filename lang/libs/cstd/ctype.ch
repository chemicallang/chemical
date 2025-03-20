
/**
 * Checks if the given character is an alphanumeric character as classified by the current C locale. In the default locale, the following characters are alphanumeric:
 * Digits (0123456789),
 * Uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ),
 * Lowercase letters (abcdefghijklmnopqrstuvwxyz).
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is an alphanumeric character, ​0​ otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isalnum
 */
@extern
public func isalnum(ch : int) : int

/**
 * Checks if the given character is an alphabetic character, i.e. either an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), or a lowercase letter (abcdefghijklmnopqrstuvwxyz).
 * In locales other than "C", an alphabetic character is a character for which isupper() or islower() returns true or any other character considered alphabetic by the locale. In any case, iscntrl(), isdigit(), ispunct() and isspace() will return false for this character.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is an alphabetic character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isalpha
 */
@extern
public func isalpha(ch : int) : int

/**
 * Checks if the given character is classified as a lowercase character according to the current C locale. In the default "C" locale, islower returns true only for the lowercase letters (abcdefghijklmnopqrstuvwxyz).
 * If islower returns true, it is guaranteed that iscntrl, isdigit, ispunct, and isspace return false for the same character in the same C locale.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a lowercase letter, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/islower
 */
@extern
public func islower(ch : int) : int

/**
 * Checks if the given character is an uppercase character according to the current C locale. In the default "C" locale, isupper returns true only for the uppercase letters (ABCDEFGHIJKLMNOPQRSTUVWXYZ).
 * If isupper returns true, it is guaranteed that iscntrl, isdigit, ispunct, and isspace return false for the same character in the same C locale.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is an uppercase letter, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isupper
 */
@extern
public func isupper(ch : int) : int

/**
 * Checks if the given character is a numeric character (0123456789).
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a numeric character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isdigit
 */
@extern
public func isdigit(ch : int) : int

/**
 * Checks if the given character is a hexadecimal numeric character (0123456789abcdefABCDEF) or is classified as a hexadecimal character.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is an hexadecimal numeric character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isxdigit
 */
@extern
public func isxdigit(ch : int) : int

/**
 * Checks if the given character is a control character, i.e. codes 0x00-0x1F and 0x7F.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a control character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/iscntrl
 */
@extern
public func iscntrl(ch : int) : int

/**
 * Checks if the given character has a graphical representation, i.e. it is either a number (0123456789), an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz), or a punctuation character (!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~), or any graphical character specific to the current C locale.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character has a graphical representation character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isgraph
 */
@extern
public func isgraph(ch : int) : int

/**
 * Checks if the given character is either
 * A standard white-space character:
 *  Space (0x20, ' '),
 *  Form feed (0x0c, '\f'),
 *  Line feed (0x0a, '\n'),
 *  Carriage return (0x0d, '\r'),
 *  Horizontal tab (0x09, '\t'),
 *  Vertical tab (0x0b, '\v'),
 * Or a locale-specific white-space character.
 *
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a whitespace character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isspace
 */
@extern
public func isspace(ch : int) : int

/**
 * Checks if the given character is a blank character in the current C locale. In the default C locale, only space (0x20) and horizontal tab (0x09) are classified as blank.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a blank character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isblank
 */
@extern
public func isblank(ch : int) : int

/**
 * Checks if the given character can be printed, i.e. it is either a number (0123456789), an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz), a punctuation character(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~), or space, or any character classified as printable by the current C locale.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character can be printed, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/isprint
 */
@extern
public func isprint(ch : int) : int

/**
 * Checks if the given character is a punctuation character in the current C locale. The default C locale classifies the characters !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ as punctuation.
 * The behavior is undefined if the value of ch is not representable as unsigned char and is not equal to EOF.
 * @return Non-zero value if the character is a punctuation character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/ispunct
 */
@extern
public func ispunct(ch : int) : int

/**
 * Converts the given character to lowercase according to the character conversion rules defined by the currently installed C locale.
 * In the default "C" locale, the following uppercase letters ABCDEFGHIJKLMNOPQRSTUVWXYZ are replaced with respective lowercase letters abcdefghijklmnopqrstuvwxyz.
 * @return Lowercase version of ch or unmodified ch if no lowercase version is listed in the current C locale.
 * @see https://en.cppreference.com/w/c/string/byte/tolower
 */
@extern
public func tolower(ch : int) : int

/**
 * Converts the given character to uppercase according to the character conversion rules defined by the currently installed C locale.
 * In the default "C" locale, the following lowercase letters abcdefghijklmnopqrstuvwxyz are replaced with respective uppercase letters ABCDEFGHIJKLMNOPQRSTUVWXYZ.
 * @return Uppercase version of ch or unmodified ch if no uppercase version is listed in the current C locale.
 * @see https://en.cppreference.com/w/c/string/byte/toupper
 */
@extern
public func toupper(ch : int) : int