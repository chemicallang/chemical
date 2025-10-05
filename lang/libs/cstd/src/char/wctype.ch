/**
 * @see https://en.cppreference.com/w/c/string/wide
 */
if(def.windows) {
    @extern
    public type wctype_t = ushort
} else {
    // TODO: unknown size, please fix this
    @extern
    public type wctype_t = u64
}


/**
 * TODO wctrans_t is implementation defined
 * @see https://en.cppreference.com/w/c/string/wide
 */
@extern
public struct wctrans_t {

}

/**
 * Checks if the given wide character is an alphanumeric character, i.e. either a number (0123456789), an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz) or any alphanumeric character specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is an alphanumeric character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswalnum
 */
@extern
public func iswalnum(ch : wint_t) : int

/**
 * Checks if the given wide character is an alphabetic character, i.e. either an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz) or any alphabetic character specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is an alphabetic character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswalpha
 */
@extern
public func iswalpha(ch : wint_t) : int

/**
 * Checks if the given wide character is a lowercase letter, i.e. one of abcdefghijklmnopqrstuvwxyz or any lowercase letter specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is an lowercase letter, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswlower
 */
@extern
public func iswlower(ch : wint_t) : int

/**
 * Checks if the given wide character is an uppercase letter, i.e. one of ABCDEFGHIJKLMNOPQRSTUVWXYZ or any uppercase letter specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is an uppercase letter, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswupper
 */
@extern
public func iswupper(ch : wint_t) : int

/**
 * Checks if the given wide character corresponds (if narrowed) to one of the ten decimal digit characters 0123456789.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a numeric character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswdigit
 */
@extern
public func iswdigit(ch : wint_t) : int

/**
 * Checks if the given wide character corresponds (if narrowed) to a hexadecimal numeric character, i.e. one of 0123456789abcdefABCDEF.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a hexadecimal numeric character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswxdigit
 */
@extern
public func iswxdigit(ch : wint_t) : int

/**
 * Checks if the given wide character is a control character, i.e. codes 0x00-0x1F and 0x7F and any control characters specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a control character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswcntrl
 */
@extern
public func iswcntrl(ch : wint_t) : int

/**
 * Checks if the given wide character has a graphical representation, i.e. it is either a number (0123456789), an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz), a punctuation character (!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~) or any graphical character specific to the current C locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character has a graphical representation character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswgraph
 */
@extern
public func iswgraph(ch : wint_t) : int

/**
 * Checks if the given wide character is a whitespace character, i.e. either space (0x20), form feed (0x0c), line feed (0x0a), carriage return (0x0d), horizontal tab (0x09), vertical tab (0x0b) or any whitespace character specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a whitespace character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswspace
 */
@extern
public func iswspace(ch : wint_t) : int

/**
 * Checks if the given wide character is classified as blank character (that is, a whitespace character used to separate words in a sentence) by the current C locale. In the default C locale, only space (0x20) and horizontal tab (0x09) are blank characters.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a blank character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswblank
 */
@extern
public func iswblank(ch : wint_t) : int

/**
 * Checks if the given wide character can be printed, i.e. it is either a number (0123456789), an uppercase letter (ABCDEFGHIJKLMNOPQRSTUVWXYZ), a lowercase letter (abcdefghijklmnopqrstuvwxyz), a punctuation character (!"#$%&'()*+,-./:;<=>?@[\]^_`{!}~), space or any printable character specific to the current C locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character can be printed, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswprint
 */
@extern
public func iswprint(ch : wint_t) : int

/**
 * Checks if the given wide character is a punctuation character, i.e. it is one of !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~ or any punctuation character specific to the current locale.
 * @param ch	-	wide character
 * @return Non-zero value if the wide character is a punctuation character, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswpunct
 */
@extern
public func iswpunct(ch : wint_t) : int

/**
 * Classifies the wide character wc using the current C locale's LC_CTYPE category identified by desc.
 * @param wc	-	the wide character to classify
 * @param desc	-	the LC_CTYPE category, obtained from a call to wctype
 * @return Non-zero if the character wc has the property identified by desc in LC_CTYPE facet of the current C locale, zero otherwise.
 * @see https://en.cppreference.com/w/c/string/wide/iswctype
 */
@extern
public func iswctype(wc : wint_t, desc : wctype_t) : int

/**
 * Constructs a value of type wctype_t that describes a LC_CTYPE category of wide character classification. It may be one of the standard classification categories, or a locale-specific category, such as "jkanji".
 * @param str	-	C string holding the name of the desired category
 * @return wctype_t object suitable for use with iswctype to classify wide characters according to the named category of the current C locale or zero if str does not name a category supported by the current C locale.
 * @see https://en.cppreference.com/w/c/string/wide/wctype
 */
@extern
public func wctype(str : *char) : wctype_t

/**
 * Converts the given wide character to lowercase, if possible.
 * @param wc	-	wide character to be converted
 * @return Lowercase version of wc or unmodified wc if no lowercase version is listed in the current C locale.
 * @see https://en.cppreference.com/w/c/string/wide/towlower
 */
@extern
public func towlower(wc : wint_t) : wint_t

/**
 * Converts the given wide character to uppercase, if possible.
 * @param wc	-	wide character to be converted
 * @return Uppercase version of wc or unmodified wc if no uppercase version is listed in the current C locale.
 * @see https://en.cppreference.com/w/c/string/wide/towupper
 */
@extern
public func towupper(wc : wint_t) : wint_t

/**
 * Maps the wide character wc using the current C locale's LC_CTYPE mapping category identified by desc.
 * @param wc	-	the wide character to map
 * @param desc	-	the LC_CTYPE mapping, obtained from a call to wctrans
 * @return The mapped value of wc using the mapping identified by desc in LC_CTYPE facet of the current C locale.
 * @see https://en.cppreference.com/w/c/string/wide/towctrans
 */
@extern
public func towctrans(wc : wint_t, desc : wctrans_t) : wint_t

/**
 * Constructs a value of type wctrans_t that describes a LC_CTYPE category of wide character mapping. It may be one of the standard mappings, or a locale-specific mapping, such as "tojhira" or "tojkata".
 * @param str	-	C string holding the name of the desired mapping.
 * The following values of str are supported in all C locales:
 *  Value of str	    Effect
 *  "toupper"	    identifies the mapping used by towupper
 *  "tolower"	    identifies the mapping used by towlower
 * @return wctrans_t object suitable for use with towctrans to map wide characters according to the named mapping of the current C locale or zero if str does not name a mapping supported by the current C locale.
 * @see https://en.cppreference.com/w/c/string/wide/wctrans
 */
@extern
public func wctrans(str : *char) : wctrans_t