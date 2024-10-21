import "./common/std_types.ch"

/**
 * Copies the null-terminated byte string pointed to by src, including the null terminator, to the character array whose first element is pointed to by dest.
 *      The behavior is undefined if the dest array is not large enough. The behavior is undefined if the strings overlap. The behavior is undefined if either dest is not a pointer to a character array or src is not a pointer to a null-terminated byte string.
 * @param dest	-	pointer to the character array to write to TODO restricted
 * @param src	-	pointer to the null-terminated byte string to copy from  TODO restricted
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/strcpy
 */
public func strcpy(dest : *mut char, src : *char) : *mut char

/**
 * Same as strcpy, except that it may clobber the rest of the destination array with unspecified values and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz is zero or greater than RSIZE_MAX
 *      destsz is less or equal strnlen_s(src, destsz); in other words, truncation would occur
 *      overlap would occur between the source and the destination strings
 * The behavior is undefined if the size of the character array pointed to by dest <= strnlen_s(src, destsz) < destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow.
 *      As with all bounds-checked functions, strcpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the character array to write to TODO restricted
 * @param src	-	pointer to the null-terminated byte string to copy from  TODO restricted
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes zero to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX).
 * @see https://en.cppreference.com/w/c/string/byte/strcpy
 */
public func strcpy_s(dest : *mut char, destsz : rsize_t, src : *char) : errno_t

/**
 * Copies at most count characters of the character array pointed to by src (including the terminating null character, but not any of the characters that follow the null character) to character array pointed to by dest.
 *  If count is reached before the entire array src was copied, the resulting character array is not null-terminated.
 *  If, after copying the terminating null character from src, count is not reached, additional null characters are written to dest until the total of count characters have been written.
 *  The behavior is undefined if the character arrays overlap, if either dest or src is not a pointer to a character array (including if dest or src is a null pointer), if the size of the array pointed to by dest is less than count, or if the size of the array pointed to by src is less than count and it does not contain a null character.
 * @param dest	-	pointer to the character array to copy to TODO restricted
 * @param src	-	pointer to the character array to copy from TODO restricted
 * @param count	-	maximum number of characters to copy
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/strncpy
 */
public func strncpy(dest : *mut char, src : *char, count : size_t) : *mut char

/**
 * Same as strncpy, except that the function does not continue writing zeroes into the destination array to pad up to count, it stops after writing the terminating null character (if there was no null in the source, it writes one at dest[count] and then stops). Also, the following errors are detected at runtime and call the currently installed constraint handler function:
 *  src or dest is a null pointer
 *  destsz is zero or greater than RSIZE_MAX
 *  count is greater than RSIZE_MAX
 *  count is greater or equal destsz, but destsz is less or equal strnlen_s(src, count), in other words, truncation would occur
 *  overlap would occur between the source and the destination strings
 *   The behavior is undefined if the size of the character array pointed to by dest < strnlen_s(src, destsz) <= destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow. The behavior is undefined if the size of the character array pointed to by src < strnlen_s(src, count) < destsz; in other words, an erroneous value of count does not expose the impending buffer overflow.
 *  As with all bounds-checked functions, strncpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the character array to copy to TODO restricted
 * @param src	-	pointer to the character array to copy from TODO restricted
 * @param count	-	maximum number of characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes zero to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX) and may clobber the rest of the destination array with unspecified values.
 * @see https://en.cppreference.com/w/c/string/byte/strncpy
 */
public func strncpy_s(dest : *mut char, destsz : rsize_t, src : *char, count : rsize_t ) : errno_t

/**
 * Appends a copy of the null-terminated byte string pointed to by src to the end of the null-terminated byte string pointed to by dest. The character src[0] replaces the null terminator at the end of dest. The resulting byte string is null-terminated.
 * The behavior is undefined if the destination array is not large enough for the contents of both src and dest and the terminating null character. The behavior is undefined if the strings overlap. The behavior is undefined if either dest or src is not a pointer to a null-terminated byte string.
 * @param dest	-	pointer to the null-terminated byte string to append to TODO restricted
 * @param src	-	pointer to the null-terminated byte string to copy from TODO restricted
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/strcat
 */
public func strcat(dest : *mut char, src : *char) : *mut char

/**
 * Same as strcat, except that it may clobber the rest of the destination array (from the last character written to destsz) with unspecified values and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz is zero or greater than RSIZE_MAX
 *      there is no null terminator in the first destsz bytes of dest
 *      truncation would occur (the available space at the end of dest would not fit every character, including the null terminator, of src)
 *      overlap would occur between the source and the destination strings
 * The behavior is undefined if the size of the character array pointed to by dest < strlen(dest)+strlen(src)+1 <= destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow.
 *      As with all bounds-checked functions, strcat_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the null-terminated byte string to append to TODO restricted
 * @param src	-	pointer to the null-terminated byte string to copy from TODO restricted
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes zero to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX).
 * @see https://en.cppreference.com/w/c/string/byte/strcat
 */
public func strcat_s(dest : *mut char, destsz : rsize_t, src : *char) : errno_t

/**
 * Appends at most count characters from the character array pointed to by src, stopping if the null character is found, to the end of the null-terminated byte string pointed to by dest. The character src[0] replaces the null terminator at the end of dest. The terminating null character is always appended in the end (so the maximum number of bytes the function may write is count+1).
 * The behavior is undefined if the destination array does not have enough space for the contents of both dest and the first count characters of src, plus the terminating null character. The behavior is undefined if the source and destination objects overlap. The behavior is undefined if either dest is not a pointer to a null-terminated byte string or src is not a pointer to a character array,
 * @param dest	-	pointer to the null-terminated byte string to append to TODO restricted
 * @param src	-	pointer to the character array to copy from TODO restricted
 * @param count	-	maximum number of characters to copy
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/strncat
 */
public func strncat(dest : *mut char, src : *char, count : size_t) : *mut char

/**
 * Same as strncat, except that this function may clobber the remainder of the destination array (from the last byte written to destsz) and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz or count is zero or greater than RSIZE_MAX
 *      there is no null character in the first destsz bytes of dest
 *      truncation would occur: count or the length of src, whichever is less, exceeds the space available between the null terminator of dest and destsz.
 *      overlap would occur between the source and the destination strings
 * The behavior is undefined if the size of the character array pointed to by dest < strnlen(dest,destsz)+strnlen(src,count)+1 < destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow. The behavior is undefined if the size of the character array pointed to by src < strnlen(src,count) < destsz; in other words, an erroneous value of count does not expose the impending buffer overflow.
 * As with all bounds-checked functions, strncat_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the null-terminated byte string to append to TODO restricted
 * @param src	-	pointer to the character array to copy from TODO restricted
 * @param count	-	maximum number of characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes zero to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX).
 * @see https://en.cppreference.com/w/c/string/byte/strncat
 */
public func strncat_s(dest : *char, destsz : rsize_t, src : *char, count : rsize_t) : errno_t

/**
 * Transforms the null-terminated byte string pointed to by src into the implementation-defined form such that comparing two transformed strings with strcmp gives the same result as comparing the original strings with strcoll, in the current C locale.
 * The first count characters of the transformed string are written to destination, including the terminating null character, and the length of the full transformed string is returned, excluding the terminating null character.
 * The behavior is undefined if the dest array is not large enough. The behavior is undefined if dest and src overlap.
 * If count is 0, then dest is allowed to be a null pointer.
 * @param dest	-	pointer to the first element of the array where the transformed string will be written TODO restricted
 * @param src	-	pointer to the first character of a null-terminated byte string to transform TODO restricted
 * @param count	-	maximum number of characters to be written
 * @return The length of the transformed string, not including the terminating null-character.
 * @see https://en.cppreference.com/w/c/string/byte/strxfrm
 */
public func strxfrm(dest : *mut char, src : *char, count : size_t) : size_t

/**
 * Returns a pointer to a null-terminated byte string, which is a duplicate of the string pointed to by src. The space for the new string is obtained as if the malloc was invoked. The returned pointer must be passed to free to avoid a memory leak.
 * If an error occurs, a null pointer is returned and errno might be set.
 * @param src	-	pointer to the null-terminated byte string to duplicate
 * @return A pointer to the newly allocated string, or a null pointer if an error occurred.
 * @see https://en.cppreference.com/w/c/string/byte/strdup
 */
public func strdup(src : *char) : *mut char

/**
 * Returns a pointer to a null-terminated byte string, which contains copies of at most size bytes from the string pointed to by src. The space for the new string is obtained as if malloc was called. If the null terminator is not encountered in the first size bytes, it is appended to the duplicated string.
 * The returned pointer must be passed to free to avoid a memory leak.
 * If an error occurs, a null pointer is returned and errno might be set.
 * @param src	-	pointer to the null-terminated byte string to duplicate
 * @param size	-	max number of bytes to copy from src
 * @return A pointer to the newly allocated string, or a null pointer if an error occurred.
 */
public func strndup(src : *char, size : size_t) : *mut char

/**
 * Returns the length of the given null-terminated byte string, that is, the number of characters in a character array whose first element is pointed to by str up to and not including the first null character.
 * The behavior is undefined if str is not a pointer to a null-terminated byte string.
 * @param str	-	pointer to the null-terminated byte string to be examined
 * @return The length of the null-terminated byte string str.
 * @see https://en.cppreference.com/w/c/string/byte/strlen
 */
public func strlen(str : *char) : size_t

/**
 * Same as strlen, except that the function returns zero if str is a null pointer and returns strsz if the null character was not found in the first strsz bytes of str.
 *  The behavior is undefined if str is not a pointer to a null-terminated byte string and strsz is greater than the size of that character array.
 *  As with all bounds-checked functions, strnlen_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param str	-	pointer to the null-terminated byte string to be examined
 * @param strsz	-	maximum number of characters to examine
 * @return The length of the null-terminated byte string str on success, zero if str is a null pointer, strsz if the null character was not found.
 * @see https://en.cppreference.com/w/c/string/byte/strlen
 */
public func strnlen_s(str : *char, strsz : size_t) : size_t

/**
 * Compares two null-terminated byte strings lexicographically.
 * The sign of the result is the sign of the difference between the values of the first pair of characters (both interpreted as unsigned char) that differ in the strings being compared.
 * The behavior is undefined if lhs or rhs are not pointers to null-terminated byte strings.
 * @param lhs, rhs	-	pointers to the null-terminated byte strings to compare
 * @return Negative value if lhs appears before rhs in lexicographical order.
 * Zero if lhs and rhs compare equal.
 * Positive value if lhs appears after rhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/byte/strcmp
 */
public func strcmp(lhs : *char, rhs : *char) : int

/**
 * Compares at most count characters of two possibly null-terminated arrays. The comparison is done lexicographically. Characters following the null character are not compared.
 * The sign of the result is the sign of the difference between the values of the first pair of characters (both interpreted as unsigned char) that differ in the arrays being compared.
 * The behavior is undefined when access occurs past the end of either array lhs or rhs. The behavior is undefined when either lhs or rhs is the null pointer.
 * @param lhs, rhs	-	pointers to the possibly null-terminated arrays to compare
 * @param count	-	maximum number of characters to compare
 * @return Negative value if lhs appears before rhs in lexicographical order.
 * Zero if lhs and rhs compare equal, or if count is zero.
 * Positive value if lhs appears after rhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/byte/strncmp
 */
public func strncmp(lhs : *char, rhs : *char, count : size_t) : int

/**
 * Compares two null-terminated byte strings according to the current locale as defined by the LC_COLLATE category.
 * @param lhs, rhs	-	pointers to the null-terminated byte strings to compare
 * @return Negative value if lhs is less than (precedes) rhs.
 * 0 if lhs is equal to rhs.
 * Positive value if lhs is greater than (follows) rhs.
 * @see https://en.cppreference.com/w/c/string/byte/strcoll
 */
public func strcoll(lhs : *char, rhs : *char) : int

/**
 * Finds the first occurrence of ch (after conversion to char as if by (char)ch) in the null-terminated byte string pointed to by str (each character interpreted as unsigned char). The terminating null character is considered to be a part of the string and can be found when searching for '\0'.
 * @param str	-	pointer to the null-terminated byte string to be analyzed
 * @param ch	-	character to search for
 * @return Pointer to the found character in str, or null pointer if no such character is found.
 * @see https://en.cppreference.com/w/c/string/byte/strchr
 */
public func strchr(str : *char, ch : int) : *mut char

/**
 * Finds the last occurrence of ch (after conversion to char as if by (char)ch) in the null-terminated byte string pointed to by str (each character interpreted as unsigned char). The terminating null character is considered to be a part of the string and can be found if searching for '\0'.
 * @param str	-	pointer to the null-terminated byte string to be analyzed
 * @param ch	-	character to search for
 * @return Pointer to the found character in str, or null pointer if no such character is found.
 * @see https://en.cppreference.com/w/c/string/byte/strrchr
 */
public func strrchr(str : *char, ch : int) : *mut char

/**
 * Returns the length of the maximum initial segment (span) of the null-terminated byte string pointed to by dest, that consists of only the characters found in the null-terminated byte string pointed to by src.
 * The behavior is undefined if either dest or src is not a pointer to a null-terminated byte string.
 * @param dest	-	pointer to the null-terminated byte string to be analyzed
 * @param src	-	pointer to the null-terminated byte string that contains the characters to search for
 * @return The length of the maximum initial segment that contains only characters from the null-terminated byte string pointed to by src.
 * @see https://en.cppreference.com/w/c/string/byte/strspn
 */
public func strspn(dest : *char, src : *char) : size_t

/**
 * Returns the length of the maximum initial segment of the null-terminated byte string pointed to by dest, that consists of only the characters not found in the null-terminated byte string pointed to by src.
 * The behavior is undefined if either dest or src is not a pointer to a null-terminated byte string.
 * @param dest	-	pointer to the null-terminated byte string to be analyzed
 * @param src	-	pointer to the null-terminated byte string that contains the characters to search for
 * @return The length of the maximum initial segment that contains only characters not found in the null-terminated byte string pointed to by src
 * @see https://en.cppreference.com/w/c/string/byte/strcspn
 */
public func strcspn(dest : *char, src : *char) : size_t

/**
 * Scans the null-terminated byte string pointed to by dest for any character from the null-terminated byte string pointed to by breakset, and returns a pointer to that character.
 * @param dest	-	pointer to the null-terminated byte string to be analyzed
 * @param breakset	-	pointer to the null-terminated byte string that contains the characters to search for
 * @return Pointer to the first character in dest, that is also in breakset, or null pointer if no such character exists.
 * @see https://en.cppreference.com/w/c/string/byte/strpbrk
 */
public func strpbrk(dest : *char, breakset : *char) : *mut char

/**
 * Finds the first occurrence of the null-terminated byte string pointed to by substr in the null-terminated byte string pointed to by str. The terminating null characters are not compared.
 * @param str	-	pointer to the null-terminated byte string to examine
 * @param substr	-	pointer to the null-terminated byte string to search for
 * @return Pointer to the first character of the found substring in str, or a null pointer if such substring is not found. If substr points to an empty string, str is returned.
 * @see https://en.cppreference.com/w/c/string/byte/strstr
 */
public func strstr(str : *char, substr : *char) : *mut char

/**
 * Finds the next token in a null-terminated byte string pointed to by str. The separator characters are identified by null-terminated byte string pointed to by delim.
 * This function is designed to be called multiple times to obtain successive tokens from the same string.
 * If str is not a null pointer, the call is treated as the first call to strtok for this particular string. The function searches for the first character which is not contained in delim.
 *      If no such character was found, there are no tokens in str at all, and the function returns a null pointer.
 *      If such character was found, it is the beginning of the token. The function then searches from that point on for the first character that is contained in delim.
 *          If no such character was found, str has only one token, and future calls to strtok will return a null pointer
 *          If such character was found, it is replaced by the null character '\0' and the pointer to the following character is stored in a static location for subsequent invocations.
 *      The function then returns the pointer to the beginning of the token
 * If str is a null pointer, the call is treated as a subsequent call to strtok: the function continues from where it left in previous invocation. The behavior is the same as if the previously stored pointer is passed as str.
 * The behavior is undefined if either str or delim is not a pointer to a null-terminated byte string.
 * @param str	-	pointer to the null-terminated byte string to tokenize
 * @param delim	-	pointer to the null-terminated byte string identifying delimiters
 * @return Returns pointer to the beginning of the next token or a null pointer if there are no more tokens.
 * @see https://en.cppreference.com/w/c/string/byte/strtok
 */
public func strtok(str : *mut char, delim : *char) : *mut char

/**
 * Same as strtok, except that on every step, writes the number of characters left to see in str into *strmax and writes the tokenizer's internal state to *ptr. Repeat calls (with null str) must pass strmax and ptr with the values stored by the previous call. Also, the following errors are detected at runtime and call the currently installed constraint handler function, without storing anything in the object pointed to by ptr
 *      strmax, delim, or ptr is a null pointer
 *      on a non-initial call (with null str), *ptr is a null pointer
 *      on the first call, *strmax is zero or greater than RSIZE_MAX
 *      search for the end of a token reaches the end of the source string (as measured by the initial value of *strmax) without encountering the null terminator
 *  The behavior is undefined if both str points to a character array which lacks the null character and strmax points to a value which is greater than the size of that character array.
 *      As with all bounds-checked functions, strtok_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 *
 * @param str	-	pointer to the null-terminated byte string to tokenize
 * @param delim	-	pointer to the null-terminated byte string identifying delimiters
 * @param strmax	-	pointer to an object which initially holds the size of str: strtok_s stores the number of characters that remain to be examined
 * @param ptr	-	pointer to an object of type char*, which is used by strtok_s to store its internal state
 * @return Returns pointer to the beginning of the next token or a null pointer if there are no more tokens.
 * @see https://en.cppreference.com/w/c/string/byte/strtok
 */
public func strtok_s(str : *mut char, strmax : *mut rsize_t, delim : *char, ptr : **mut char) : *mut char

/**
 * Finds the first occurrence of (unsigned char)ch in the initial count bytes (each interpreted as unsigned char) of the object pointed to by ptr.
 * @param ptr	-	pointer to the object to be examined
 * @param ch	-	bytes to search for
 * @param count	-	max number of bytes to examine
 * @return Pointer to the location of the byte, or a null pointer if no such byte is found.
 * @see https://en.cppreference.com/w/c/string/byte/memchr
 */
public func memchr(ptr : *void, ch : int, count : size_t) : *mut void

/**
 * Compares the first count bytes of the objects pointed to by lhs and rhs. The comparison is done lexicographically.
 * The sign of the result is the sign of the difference between the values of the first pair of bytes (both interpreted as unsigned char) that differ in the objects being compared.
 * The behavior is undefined if access occurs beyond the end of either object pointed to by lhs and rhs. The behavior is undefined if either lhs or rhs is a null pointer.
 * @param lhs, rhs	-	pointers to the objects to compare
 * @param count	-	number of bytes to examine
 * @return Negative value if lhs appears before rhs in lexicographical order.
 * Zero if lhs and rhs compare equal, or if count is zero.
 * Positive value if lhs appears after rhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/byte/memcmp
 */
public func memcmp(lhs : *void, rhs : *void, count : size_t) : int

/**
 * Copies the value (unsigned char)ch into each of the first count characters of the object pointed to by dest.
 * The behavior is undefined if access occurs beyond the end of the dest array. The behavior is undefined if dest is a null pointer.
 * @param dest	-	pointer to the object to fill
 * @param ch	-	fill byte
 * @param count	-	number of bytes to fill
 * @return A copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/memset
 */
public func memset(dest : *mut void, ch : int, count : size_t) : *mut void

/**
 * Same as memset, except that is safe for sensitive information.
 * @param dest	-	pointer to the object to fill
 * @param ch	-	fill byte
 * @param count	-	number of bytes to fill
 * @return A copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/memset
 */
public func memset_explicit(dest : *mut void, ch : int, count : size_t) : *mut void

/**
 * Same as (1), except that the following errors are detected at runtime and call the currently installed constraint handler function after storing ch in every location of the destination range [dest, dest+destsz) if dest and destsz are themselves valid:
 *      dest is a null pointer
 *      destsz or count is greater than RSIZE_MAX
 *      count is greater than destsz (buffer overflow would occur)
 * The behavior is undefined if the size of the character array pointed to by dest < count <= destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow.
 *  As with all bounds-checked functions, memset_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the object to fill
 * @param ch	-	fill byte
 * @param count	-	number of bytes to fill
 * @param destsz	-	size of the destination array
 * @return zero on success, non-zero on error. Also on error, if dest is not a null pointer and destsz is valid, writes destsz fill bytes ch to the destination array.
 * @see https://en.cppreference.com/w/c/string/byte/memset
 */
public func memset_s(dest : *mut void, destsz : rsize_t, ch : int, count : rsize_t) : errno_t

/**
 * Copies count characters from the object pointed to by src to the object pointed to by dest. Both objects are interpreted as arrays of unsigned char.
 * The behavior is undefined if access occurs beyond the end of the dest array. If the objects overlap (which is a violation of the restrict contract)(since C99), the behavior is undefined. The behavior is undefined if either dest or src is an invalid or null pointer.
 * @param dest	-	pointer to the object to copy to TODO restricted
 * @param src	-	pointer to the object to copy from TODO restricted
 * @param count	-	number of bytes to copy
 * @return Returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/memcpy
 */
public func memcpy(dest : *mut void, src : *void, count : size_t) : *mut void

/**
 * Same as memcpy, except that the following errors are detected at runtime and cause the entire destination range [dest, dest+destsz) to be zeroed out (if both dest and destsz are valid), as well as call the currently installed constraint handler function:
 *      dest or src is a null pointer
 *      destsz or count is greater than RSIZE_MAX
 *      count is greater than destsz (buffer overflow would occur)
 *      the source and the destination objects overlap
 * The behavior is undefined if the size of the character array pointed to by dest < count <= destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow.
 *  As with all bounds-checked functions, memcpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the object to copy to TODO restricted
 * @param destsz	-	max number of bytes to modify in the destination (typically the size of the destination object)
 * @param src	-	pointer to the object to copy from TODO restricted
 * @param count	-	number of bytes to copy
 * @return Returns zero on success and non-zero value on error. Also on error, if dest is not a null pointer and destsz is valid, writes destsz zero bytes in to the destination array.
 * @see https://en.cppreference.com/w/c/string/byte/memcpy
 */
public func memcpy_s(dest : *mut void, destsz : rsize_t, src : *void, count : rsize_t) : errno_t

/**
 * Copies count characters from the object pointed to by src to the object pointed to by dest. Both objects are interpreted as arrays of unsigned char. The objects may overlap: copying takes place as if the characters were copied to a temporary character array and then the characters were copied from the array to dest.
 *  The behavior is undefined if access occurs beyond the end of the dest array. The behavior is undefined if either dest or src is an invalid or null pointer.
 * @param dest	-	pointer to the object to copy to
 * @param destsz	-	max number of bytes to modify in the destination (typically the size of the destination object)
 * @param src	-	pointer to the object to copy from
 * @param count	-	number of bytes to copy
 * @return Returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/byte/memmove
 */
public func memmove(dest : *mut void, src : *void, count : size_t) : *mut void

/**
 * Same as memmove, except when detecting the following errors at runtime, it zeroes out the entire destination range [dest, dest+destsz) (if both dest and destsz are valid) and calls the currently installed constraint handler function:
 *      dest or src is a null pointer
 *      destsz or count is greater than RSIZE_MAX
 *      count is greater than destsz (buffer overflow would occur)
 * The behavior is undefined if the size of the character array pointed to by dest < count <= destsz; in other words, an erroneous value of destsz does not expose the impending buffer overflow.
 *  As with all bounds-checked functions, memmove_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param dest	-	pointer to the object to copy to
 * @param destsz	-	max number of bytes to modify in the destination (typically the size of the destination object)
 * @param src	-	pointer to the object to copy from
 * @param count	-	number of bytes to copy
 * @return Returns zero on success and non-zero value on error. Also on error, if dest is not a null pointer and destsz is valid, writes destsz zero bytes in to the destination array.
 * @see https://en.cppreference.com/w/c/string/byte/memmove
 */
public func memmove_s(dest : *mut void, destsz : rsize_t, src : *void, count : rsize_t) : errno_t

/**
 * Copies bytes from the object pointed to by src to the object pointed to by dest, stopping after any of the next two conditions are satisfied:
 *      count bytes are copied
 *      the byte (unsigned char)c is found (and copied).
 * The src and dest objects are interpreted as arrays of unsigned char.
 * The behavior is undefined if any condition is met:
 *      access occurs beyond the end of the dest array;
 *      the objects overlap (which is a violation of the restrict contract)
 *      either dest or src is an invalid or null pointer
 * @param dest	-	pointer to the object to copy to TODO restricted
 * @param src	-	pointer to the object to copy from TODO restricted
 * @param c	-	terminating byte, converted to unsigned char at first
 * @param count	-	number of bytes to copy
 * @return If the byte (unsigned char)c was found, memccpy returns a pointer to the next byte in dest after (unsigned char)c. Otherwise it returns a null pointer.
 */
public func memccpy(dest : *mut void, src : *void, c : int, count : size_t) : *mut void

/**
 * Returns a pointer to the textual description of the system error code errnum, identical to the description that would be printed by perror().
 *   errnum is usually acquired from the errno variable, however the function accepts any value of type int. The contents of the string are locale-specific.
 *   The returned string must not be modified by the program, but may be overwritten by a subsequent call to the strerror function. strerror is not required to be thread-safe. Implementations may be returning different pointers to static read-only string literals or may be returning the same pointer over and over, pointing at a static buffer in which strerror places the string.
 * @param errnum	-	integral value referring to an error code
 * @param buf	-	pointer to a user-provided buffer
 * @param bufsz	-	size of the user-provided buffer
 * @return Pointer to a null-terminated byte string corresponding to the errno error code errnum.
 * @see https://en.cppreference.com/w/c/string/byte/strerror
 */
public func strerror(errnum : int) : *mut char

/**
 * Same as (1), except that the message is copied into user-provided storage buf. No more than bufsz-1 bytes are written, the buffer is always null-terminated. If the message had to be truncated to fit the buffer and bufsz is greater than 3, then only bufsz-4 bytes are written, and the characters "..." are appended before the null terminator. In addition, the following errors are detected at runtime and call the currently installed constraint handler function:
 *  buf is a null pointer
 *  bufsz is zero or greater than RSIZE_MAX
 *   The behavior is undefined if writing to buf occurs past the end of the array, which can happen when the size of the buffer pointed to by buf is less than the number of characters in the error message which in turn is less than bufsz.
 * @param errnum	-	integral value referring to an error code
 * @param buf	-	pointer to a user-provided buffer
 * @param bufsz	-	size of the user-provided buffer
 * @return Zero if the entire message was successfully stored in buf, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/string/byte/strerror
 */
public func strerror_s(buf : *mut char, bufsz : rsize_t, errnum : errno_t) : errno_t

/**
 * Computes the length of the untruncated locale-specific error message that strerror_s would write if it were called with errnum. The length does not include the null terminator.
 *  As with all bounds-checked functions, strerror_s and strerrorlen_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <string.h>.
 * @param errnum	-	integral value referring to an error code
 * @param buf	-	pointer to a user-provided buffer
 * @param bufsz	-	size of the user-provided buffer
 * @return Length (not including the null terminator) of the message that strerror_s would return
 * @see https://en.cppreference.com/w/c/string/byte/strerror
 */
public func strerrorlen_s(errnum : errno_t) : size_t