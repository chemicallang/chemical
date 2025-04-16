/**
 * Converts a narrow multibyte character to UTF-8 encoding.
 * If s is not a null pointer, inspects at most n bytes of the multibyte character string, beginning with the byte pointed to by s to determine the number of bytes necessary to complete the next multibyte character (including any shift sequences). If the function determines that the next multibyte character in s is complete and valid, converts it to UTF-8 and stores the first UTF-8 code unit in *pc8 (if pc8 is not null).
 * If UTF-8 encoding of the multibyte character in *s consists of more than one UTF-8 code unit, then after the first call to this function, *ps is updated in such a way that the next call to mbrtoc8 will write out the additional UTF-8 code units, without considering *s.
 * If s is a null pointer, the values of n and pc8 are ignored and the call is equivalent to mbrtoc8(nullptr, "", 1, ps).
 * If UTF-8 code unit produced is u8'\0', the conversion state *ps represents the initial shift state.
 * The multibyte encoding used by this function is specified by the currently active C locale.
 * @param pc8	-	pointer to the location where the resulting UTF-8 code units will be written
 * @param s	-	pointer to the multibyte character string used as input
 * @param n	-	limit on the number of bytes in s that can be examined
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return The first of the following that applies:
 *         0 if the character converted from s (and stored in *pc8 if non-null) was the null character
 *         the number of bytes [1...n] of the multibyte character successfully converted from s
 *         (size_t)-3 if the next UTF-8 code unit from a character whose encoding consists of multiple code units has now been written to *pc8. No bytes are processed from the input in this case.
 *         (size_t)-2 if the next n bytes constitute an incomplete, but so far valid, multibyte character. Nothing is written to *pc8.
 *         (size_t)-1 if encoding error occurs. Nothing is written to *pc8, the value EILSEQ is stored in errno and the value of *ps is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbrtoc8
 */
@extern
public func mbrtoc8(pc8 : *mut char8_t, s : *char, n : size_t, ps : *mut mbstate_t) : size_t

/**
 * Converts a single code point from UTF-8 to a narrow multibyte character representation.
 * If s is not a null pointer and c8 is the last code unit in a valid UTF-8 encoding of a code point, the function determines the number of bytes necessary to store the multibyte character representation of that code point (including any shift sequences, and taking into account the current multibyte conversion state *ps), and stores the multibyte character representation in the character array whose first element is pointed to by s, updating *ps as necessary. At most MB_CUR_MAX bytes can be written by this function.
 * If c8 is not the final UTF-8 code unit in a representation of a code point, the function does not write to the array pointed to by s, only *ps is updated.
 * If s is a null pointer, the call is equivalent to c8rtomb(buf, u8'\0', ps) for some internal buffer buf.
 * If c8 is the null character u8'\0', a null byte is stored, preceded by any shift sequence necessary to restore the initial shift state and the conversion state parameter *ps is updated to represent the initial shift state.
 * The multibyte encoding used by this function is specified by the currently active C locale.
 * @param s	-	pointer to narrow character array where the multibyte character will be stored
 * @param c8	-	the UTF-8 code unit to convert
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return The number of bytes stored in the array object (including any shift sequences). This may be zero when c8 is not the final code unit in the UTF-8 representation of a code point.
 *         If c8 is invalid (does not contribute to a sequence of char8_t corresponding to a valid multibyte character), the value of the macro EILSEQ is stored in errno, (size_t)-1 is returned, and the conversion state is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/c8rtomb
 */
@extern
public func c8rtomb(s : *mut char, c8 : char8_t, ps : *mut mbstate_t) : size_t

/**
 * Converts a single code point from its narrow multibyte character representation to its variable-length 16-bit wide character representation (typically, UTF-16).
 * If s is not a null pointer, inspects at most n bytes of the multibyte character string, beginning with the byte pointed to by s to determine the number of bytes necessary to complete the next multibyte character (including any shift sequences, and taking into account the current multibyte conversion state *ps). If the function determines that the next multibyte character in s is complete and valid, converts it to the corresponding 16-bit wide character and stores it in *pc16 (if pc16 is not null).
 * If the multibyte character in *s corresponds to a multi-char16_t sequence (e.g. a surrogate pair in UTF-16), then after the first call to this function, *ps is updated in such a way that the next call to mbrtoc16 will write out the additional char16_t, without considering *s.
 * If s is a null pointer, the values of n and pc16 are ignored and the call is equivalent to mbrtoc16(NULL, "", 1, ps).
 * If the wide character produced is the null character, the conversion state *ps represents the initial shift state.
 * If the macro __STDC_UTF_16__ is defined, the 16-bit encoding used by this function is UTF-16; otherwise, it is implementation-defined. The macro is always defined and the encoding is always UTF-16.(since C23) In any case, the multibyte character encoding used by this function is specified by the currently active C locale.
 * @param pc16	-	pointer to the location where the resulting 16-bit wide character will be written
 * @param s	-	pointer to the multibyte character string used as input
 * @param n	-	limit on the number of bytes in s that can be examined
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return The first of the following that applies:
 *         0 if the character converted from s (and stored in *pc16 if non-null) was the null character
 *         the number of bytes [1...n] of the multibyte character successfully converted from s
 *         (size_t)-3 if the next char16_t from a multi-char16_t character (e.g. a surrogate pair) has now been written to *pc16. No bytes are processed from the input in this case.
 *         (size_t)-2 if the next n bytes constitute an incomplete, but so far valid, multibyte character. Nothing is written to *pc16.
 *         (size_t)-1 if encoding error occurs. Nothing is written to *pc16, the value EILSEQ is stored in errno and the value of *ps is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbrtoc16
 */
@extern
public func mbrtoc16(pc16 : *mut char16_t, s : *char, n : size_t, ps : *mut mbstate_t) : size_t

/**
 * Converts a single code point from its variable-length 16-bit wide character representation (typically, UTF-16) to its narrow multibyte character representation.
 * If s is not a null pointer and c16 is the last 16-bit code unit in a valid variable-length encoding of a code point, the function determines the number of bytes necessary to store the multibyte character representation of that code point (including any shift sequences, and taking into account the current multibyte conversion state *ps), and stores the multibyte character representation in the character array whose first element is pointed to by s, updating *ps as necessary. At most MB_CUR_MAX bytes can be written by this function.
 * If s is a null pointer, the call is equivalent to c16rtomb(buf, u'\0', ps) for some internal buffer buf.
 * If c16 is the null wide character u'\0', a null byte is stored, preceded by any shift sequence necessary to restore the initial shift state and the conversion state parameter *ps is updated to represent the initial shift state.
 * If c16 is not the final code unit in a 16-bit representation of a wide character, it does not write to the array pointed to by s, only *ps is updated.
 * If the macro __STDC_UTF_16__ is defined, the 16-bit encoding used by this function is UTF-16; otherwise, it is implementation-defined. The macro is always defined and the encoding is always UTF-16.(since C23) In any case, the multibyte character encoding used by this function is specified by the currently active C locale.
 * @param s	-	pointer to narrow character array where the multibyte character will be stored
 * @param c16	-	the 16-bit wide character to convert
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return On success, returns the number of bytes (including any shift sequences) written to the character array whose first element is pointed to by s. This value may be ​0​, e.g. when processing the leading char16_t units in a multi-char16_t-unit sequence (occurs when processing the leading surrogate in a surrogate pair of UTF-16).
 *         On failure (if c16 is not a valid 16-bit code unit), returns -1, stores EILSEQ in errno, and leaves *ps in unspecified state.
 * @see https://en.cppreference.com/w/c/string/multibyte/c16rtomb
 */
@extern
public func c16rtomb(s : *mut char, c16 : char16_t, ps : *mut mbstate_t) : size_t

/**
 * Converts a single code point from its narrow multibyte character representation to its variable-length 32-bit wide character representation (but typically, UTF-32).
 * If s is not a null pointer, inspects at most n bytes of the multibyte character string, beginning with the byte pointed to by s to determine the number of bytes necessary to complete the next multibyte character (including any shift sequences, and taking into account the current multibyte conversion state *ps). If the function determines that the next multibyte character in s is complete and valid, converts it to the corresponding 32-bit wide character and stores it in *pc32 (if pc32 is not null).
 * If the multibyte character in *s corresponds to a multi-char32_t sequence (not possible with UTF-32), then after the first call to this function, *ps is updated in such a way that the next calls to mbrtoc32 will write out the additional char32_t, without considering *s.
 * If s is a null pointer, the values of n and pc32 are ignored and the call is equivalent to mbrtoc32(NULL, "", 1, ps).
 * If the wide character produced is the null character, the conversion state *ps represents the initial shift state.
 * If the macro __STDC_UTF_32__ is defined, the 32-bit encoding used by this function is UTF-32; otherwise, it is implementation-defined. The macro is always defined and the encoding is always UTF-32.(since C23) In any case, the multibyte character encoding used by this function is specified by the currently active C locale.
 * @param pc32	-	pointer to the location where the resulting 32-bit wide character will be written
 * @param s	-	pointer to the multibyte character string used as input
 * @param n	-	limit on the number of bytes in s that can be examined
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return The first of the following that applies:
 *         0 if the character converted from s (and stored in *pc32 if non-null) was the null character
 *         the number of bytes [1...n] of the multibyte character successfully converted from s
 *         (size_t)-3 if the next char32_t from a multi-char32_t character has now been written to *pc32. No bytes are processed from the input in this case.
 *         (size_t)-2 if the next n bytes constitute an incomplete, but so far valid, multibyte character. Nothing is written to *pc32.
 *         (size_t)-1 if encoding error occurs. Nothing is written to *pc32, the value EILSEQ is stored in errno and the value of *ps is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbrtoc32
 */
@extern
public func mbrtoc32(pc32 : *mut char32_t, s : *char, n : size_t, ps : *mut mbstate_t) : size_t

/**
 * Converts a single code point from its variable-length 32-bit wide character representation (but typically, UTF-32) to its narrow multibyte character representation.
 * If s is not a null pointer, the function determines the number of bytes necessary to store the multibyte character representation of c32 (including any shift sequences, and taking into account the current multibyte conversion state *ps), and stores the multibyte character representation in the character array whose first element is pointed to by s, updating *ps as necessary. At most MB_CUR_MAX bytes can be written by this function.
 * If s is a null pointer, the call is equivalent to c32rtomb(buf, U'\0', ps) for some internal buffer buf.
 * If c32 is the null wide character U'\0', a null byte is stored, preceded by any shift sequence necessary to restore the initial shift state and the conversion state parameter *ps is updated to represent the initial shift state.
 * If the macro __STDC_UTF_32__ is defined, the 32-bit encoding used by this function is UTF-32; otherwise, it is implementation-defined. The macro is always defined and the encoding is always UTF-32.(since C23) In any case, the multibyte character encoding used by this function is specified by the currently active C locale.
 * @param s	-	pointer to narrow character array where the multibyte character will be stored
 * @param c32	-	the 32-bit wide character to convert
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @return On success, returns the number of bytes (including any shift sequences) written to the character array whose first element is pointed to by s. This value may be ​0​, e.g. when processing the leading char32_t units in a multi-char32_t-unit sequence (does not occur in UTF-32).
 *         On failure (if c32 is not a valid 32-bit wide character), returns -1, stores EILSEQ in errno, and leaves *ps in unspecified state.
 * @see https://en.cppreference.com/w/c/string/multibyte/c32rtomb
 */
@extern
public func c32rtomb(s : *mut char, c32 : char32_t, ps : *mut mbstate_t) : size_t
