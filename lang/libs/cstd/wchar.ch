/**
 * If mode > 0, attempts to make stream wide-oriented. If mode < 0, attempts to make stream byte-oriented. If mode==0, only queries the current orientation of the stream.
 * If the orientation of the stream has already been decided (by executing output or by an earlier call to fwide), this function does nothing.
 * @param stream	-	pointer to the C I/O stream to modify or query
 * @param mode	-	integer value greater than zero to set the stream wide, less than zero to set the stream narrow, or zero to query only
 * @return An integer greater than zero if the stream is wide-oriented after this call, less than zero if the stream is byte-oriented after this call, and zero if the stream has no orientation.
 * @see https://en.cppreference.com/w/c/io/fwide
 */
public func fwide(stream : *mut FILE, mode : int) : int

/**
 * TODO wint_t is implementation defined
 */
typealias wint_t = short;
/**
 * TODO wchar_t is implementation defined
 */
typealias wchar_t = short;

/**
 * Reads the next wide character from the given input stream. getwc() may be implemented as a macro and may evaluate stream more than once.
 * @param stream	-	to read the wide character from
 * @return The next wide character from the stream or WEOF on failure.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stream. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fgetwc
 */
public func fgetwc(stream : *mut FILE) : wint_t

/**
 * Reads the next wide character from the given input stream. getwc() may be implemented as a macro and may evaluate stream more than once.
 * @param stream	-	to read the wide character from
 * @return The next wide character from the stream or WEOF on failure.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stream. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fgetwc
 */
public func getwc(stream : *mut FILE) : wint_t

/**
 * Reads at most count - 1 wide characters from the given file stream and stores them in str. The produced wide string is always null-terminated. Parsing stops if end-of-file occurs or a newline wide character is found, in which case str will contain that wide newline character.
 * @param str	-	wide string to read the characters to
 * @param count	-	the length of str
 * @param stream	-	file stream to read the data from
 * @return str on success, a null pointer on an error
 * @see https://en.cppreference.com/w/c/io/fgetws
 */
public func fgetws(str : *mut wchar_t, count : int, stream : *mut FILE) : *mut wchar_t

/**
 * Reads at most count - 1 wide characters from the given file stream and stores them in str. The produced wide string is always null-terminated. Parsing stops if end-of-file occurs or a newline wide character is found, in which case str will contain that wide newline character.
 * @param str	-	wide string to read the characters to
 * @param count	-	the length of str
 * @param stream	-	file stream to read the data from
 * @return str on success, a null pointer on an error
 * @see https://en.cppreference.com/w/c/io/fgetws
 */
public func fgetws(str : *mut wchar_t, count : int, stream : *mut FILE) : *mut wchar_t

/**
 * Writes a wide character ch to the given output stream stream.
 * @param ch	-	wide character to be written
 * @param stream	-	the output stream
 * @return Returns a copy of ch on success.
 *         On failure, returns WEOF and sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fputwc
 */
public func fputwc(ch : wchar_t, stream : *mut FILE) : wint_t

/**
 * Writes a wide character ch to the given output stream stream.
 * @param ch	-	wide character to be written
 * @param stream	-	the output stream
 * @return Returns a copy of ch on success.
 *         On failure, returns WEOF and sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fputwc
 */
public func putwc(ch : wchar_t, stream : *mut FILE) : wint_t

/**
 * Writes every character from the null-terminated wide string str to the output stream stream, as if by repeatedly executing fputwc.
 * The terminating null wide character from str is not written.
 * @param str	-	null-terminated wide string to be written
 * @param stream	-	output stream
 * @return On success, returns a non-negative value
 *         On failure, returns EOF and sets the error indicator (see ferror) on stream.
 * @see https://en.cppreference.com/w/c/io/fputws
 */
public func fputws(str : *wchar_t, stream : *mut FILE) : int

/**
 * Reads the next wide character from stdin.
 * @return the obtained wide character or WEOF if an error has occurred or the end of file reached
 * @see https://en.cppreference.com/w/c/io/getwchar
 */
public func getwchar() : wint_t

/**
 * Writes a wide character ch to stdout.
 * @param ch	-	wide character to be written
 * @return ch on success, WEOF on failure.
 * @see https://en.cppreference.com/w/c/io/putwchar
 */
public func putwchar(ch : wchar_t) : wint_t

/**
 * If ch does not equal WEOF, pushes the wide character ch into the input buffer associated with the stream stream in such a manner that subsequent read operation from stream will retrieve that wide character. The external device associated with the stream is not modified.
 * Stream repositioning operations fseek, fsetpos, and rewind discard the effects of ungetwc.
 * If ungetwc is called more than once without an intervening read or repositioning, it may fail (in other words, a pushback buffer of size 1 is guaranteed, but any larger buffer is implementation-defined). If multiple successful ungetwc were performed, read operations retrieve the pushed-back wide characters in reverse order of ungetwc
 * If ch equals WEOF, the operation fails and the stream is not affected.
 * A successful call to ungetwc clears the end of file status flag feof.
 * A successful call to ungetwc on a stream (whether text or binary) modifies the stream position indicator in unspecified manner but guarantees that after all pushed-back wide characters are retrieved with a read operation, the stream position indicator is equal to its value before ungetwc.
 * @param ch	-	wide character to be put back
 * @param stream	-	file stream to put the wide character back to
 * @return On success ch is returned.
 *         On failure WEOF is returned and the given stream remains unchanged.
 * @see https://en.cppreference.com/w/c/io/ungetwc
 */
public func ungetwc(ch : wint_t, stream : *mut FILE) : wint_t

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Reads the data from stdin.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func wscanf(format : *wchar_t, ... ) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Reads the data from file stream stream.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func fwscanf(stream : *mut FILE, format : *wchar_t, ... ) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Reads the data from null-terminated wide string buffer. Reaching the end of the string is equivalent to reaching the end-of-file condition for fwscanf
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func swscanf(buffer : *wchar_t, format : *wchar_t, ... ) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As all bounds-checked functions, wscanf_s, fwscanf_s, and swscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func wscanf_s(format : *wchar_t, ...) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As all bounds-checked functions, wscanf_s, fwscanf_s, and swscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func fwscanf_s(stream : *mut FILE, format : *wchar_t, ...) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As all bounds-checked functions, wscanf_s, fwscanf_s, and swscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fwscanf
 */
public func swscanf_s( s : *wchar_t, format : *wchar_t, ...) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from stdin.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vwscanf(format : *wchar_t, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from file stream stream.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vfwscanf(stream : *mut FILE, format : *wchar_t, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from null-terminated wide string buffer. Reaching the end of the string is equivalent to reaching the end-of-file condition for fwscanf
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vswscanf(buffer : *wchar_t, format : *wchar_t, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vwscanf_s, vfwscanf_s, and vswscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vwscanf_s(format : *wchar_t, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vwscanf_s, vfwscanf_s, and vswscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vfwscanf_s(stream : *mut FILE, format : *wchar_t, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %lc into a single wide character) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      any of the arguments of pointer type is a null pointer
 *      format, stream, or buffer is a null pointer
 *      the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *      optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vwscanf_s, vfwscanf_s, and vswscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated wide string to read from
 * @param format	-	pointer to a null-terminated wide string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *      non-whitespace wide characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *      whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling iswspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *      conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *          (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *          (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *          conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfwscanf
 */
public func vswscanf_s(buffer : *wchar_t, format : *wchar_t, vlist : va_list) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Writes the results to stdout.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func wprintf(format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Writes the results to a file stream stream
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func fwprintf(stream : *mut FILE, format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * If bufsz is greater than zero, writes the results to a wide string buffer. At most bufsz - 1 wide characters are written followed by null wide character. If bufsz is zero, nothing is written (and buffer may be a null pointer).
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters written (not counting the terminating null wide character) if successful or negative value if an encoding error occurred or if the number of characters to be generated was equal or greater than bufsz (including when bufsz is zero).
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func swprintf(buffer : *mut wchar_t, bufsz : size_t, format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (only for swprintf_s) the number of wide characters to be written, including the null, would exceed bufsz
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func wprintf_s(format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (only for swprintf_s) the number of wide characters to be written, including the null, would exceed bufsz
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func fwprintf_s(stream : *mut FILE, format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (only for swprintf_s) the number of wide characters to be written, including the null, would exceed bufsz
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters (not counting the terminating null) that were written to buffer. Returns a negative value on encoding errors and on overflow. Returns zero on all other errors.
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func swprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, ... ) : int

/**
 * Loads the data from the given locations, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (6), except it will truncate the result to fit within the array pointed to by s.
 *      As with all bounds-checked functions, wprintf_s, fwprintf_s, swprintf_s, and snwprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a wide character string to write to
 * @param bufsz	-	up to bufsz - 1 wide characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return Number of wide characters (not counting the terminating null) that would have been written to buffer had bufsz been sufficiently large, or a negative value if an error occurs. (meaning, write was successful and complete only if the return is nonnegative and less than bufsz)
 * @see https://en.cppreference.com/w/c/io/fwprintf
 */
public func snwprintf_s(s : *mut wchar_t, n : rsize_t, format : *wchar_t, ... ) : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Writes the results to stdout.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return The number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vwprintf(format : *wchar_t, vlist : va_list) : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Writes the results to a file stream stream.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return number of wide characters transmitted to the output stream or negative value if an output error, a runtime constraints violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vfwprintf(stream : *mut FILE, format : *wchar_t, vlist : va_list) : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Writes the results to a wide string buffer. At most bufsz - 1 wide characters are written followed by null wide character. The resulting wide character string will be terminated with a null wide character, unless bufsz is zero.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return The number of wide characters written if successful or negative value if an error occurred. If the resulting string gets truncated due to bufsz limit, function returns the total number of characters (not including the terminating null wide character) which would have been written, if the limit were not imposed.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vswprintf(buffer : *mut wchar_t, bufsz : size_t, format : *wchar_t, vlist : va_list) : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vswprintf_s only), the string to be stored in buffer (including the trailing wide null) would be exceed bufsz.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return The number of wide characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vwprintf_s(format : *wchar_t, vlist : va_list : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vswprintf_s only), the string to be stored in buffer (including the trailing wide null) would be exceed bufsz.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return number of wide characters transmitted to the output stream or negative value if an output error, a runtime constraints violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vfwprintf_s(stream : *mut FILE, format : *wchar_t, vlist : va_list : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (1-3), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vswprintf_s only), the string to be stored in buffer (including the trailing wide null) would be exceed bufsz.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return number of wide characters written to buffer, not counting the null wide character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX/sizeof(wchar_t)), or zero on runtime constraint violations, and negative value on encoding errors.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vswprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, vlist : va_list : int

/**
 * Loads the data from locations, defined by vlist, converts them to wide string equivalents and writes the results to a variety of sinks.
 * Same as (6), except it will truncate the result to fit within the array pointed to by buffer.
 *      As with all bounds-checked functions, vwprintf_s, vfwprintf_s, vswprintf_s, and vsnwprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output wide stream to write to
 * @param buffer	-	pointer to a wide string to write to
 * @param bufsz	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary wide characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *      introductory % character.
 *      (optional) one or more flags that modify the behavior of the conversion:
 *          -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *          +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *          space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *          #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *          0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *      (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *      (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *      (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *      conversion format specifier.
 * @return number of wide characters not including the terminating null character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX/sizeof(wchar_t)), which would have been written to buffer if bufsz was ignored, or a negative value if a runtime constraints violation or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/vfwprintf
 */
public func vsnwprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, vlist : va_list : int