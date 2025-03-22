import "./common/std_types.ch"
import "./common/wchar_types.ch"
import "./common/multibyte_char_types.ch"
import "./common/io_types.ch"
import "./common/arg_types.ch"
import "./common/time_types.ch"

/**
 * If mode > 0, attempts to make stream wide-oriented. If mode < 0, attempts to make stream byte-oriented. If mode==0, only queries the current orientation of the stream.
 * If the orientation of the stream has already been decided (by executing output or by an earlier call to fwide), this function does nothing.
 * @param stream	-	pointer to the C I/O stream to modify or query
 * @param mode	-	integer value greater than zero to set the stream wide, less than zero to set the stream narrow, or zero to query only
 * @return An integer greater than zero if the stream is wide-oriented after this call, less than zero if the stream is byte-oriented after this call, and zero if the stream has no orientation.
 * @see https://en.cppreference.com/w/c/io/fwide
 */
@extern
public func fwide(stream : *mut FILE, mode : int) : int

/**
 * Reads the next wide character from the given input stream. getwc() may be implemented as a macro and may evaluate stream more than once.
 * @param stream	-	to read the wide character from
 * @return The next wide character from the stream or WEOF on failure.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stream. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fgetwc
 */
@extern
public func fgetwc(stream : *mut FILE) : wint_t

/**
 * Reads the next wide character from the given input stream. getwc() may be implemented as a macro and may evaluate stream more than once.
 * @param stream	-	to read the wide character from
 * @return The next wide character from the stream or WEOF on failure.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stream. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream.
 *         If an encoding error occurred, additionally sets errno to EILSEQ.
 * @see https://en.cppreference.com/w/c/io/fgetwc
 */
@extern
public func getwc(stream : *mut FILE) : wint_t

/**
 * Reads at most count - 1 wide characters from the given file stream and stores them in str. The produced wide string is always null-terminated. Parsing stops if end-of-file occurs or a newline wide character is found, in which case str will contain that wide newline character.
 * @param str	-	wide string to read the characters to
 * @param count	-	the length of str
 * @param stream	-	file stream to read the data from
 * @return str on success, a null pointer on an error
 * @see https://en.cppreference.com/w/c/io/fgetws
 */
@extern
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
@extern
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
@extern
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
@extern
public func fputws(str : *wchar_t, stream : *mut FILE) : int

/**
 * Reads the next wide character from stdin.
 * @return the obtained wide character or WEOF if an error has occurred or the end of file reached
 * @see https://en.cppreference.com/w/c/io/getwchar
 */
@extern
public func getwchar() : wint_t

/**
 * Writes a wide character ch to stdout.
 * @param ch	-	wide character to be written
 * @return ch on success, WEOF on failure.
 * @see https://en.cppreference.com/w/c/io/putwchar
 */
@extern
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
@extern
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
@extern
public func wscanf(format : *wchar_t, _ : any... ) : int

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
@extern
public func fwscanf(stream : *mut FILE, format : *wchar_t, _ : any... ) : int

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
@extern
public func swscanf(buffer : *wchar_t, format : *wchar_t, _ : any... ) : int

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
@extern
public func wscanf_s(format : *wchar_t, _ : any...) : int

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
@extern
public func fwscanf_s(stream : *mut FILE, format : *wchar_t, _ : any...) : int

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
@extern
public func swscanf_s( s : *wchar_t, format : *wchar_t, _ : any...) : int

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
@extern
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
@extern
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
@extern
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
@extern
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
@extern
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
@extern
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
@extern
public func wprintf(format : *wchar_t, _ : any...) : int

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
@extern
public func fwprintf(stream : *mut FILE, format : *wchar_t, _ : any...) : int

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
@extern
public func swprintf(buffer : *mut wchar_t, bufsz : size_t, format : *wchar_t, _ : any...) : int

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
@extern
public func wprintf_s(format : *wchar_t, _ : any...) : int

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
@extern
public func fwprintf_s(stream : *mut FILE, format : *wchar_t, _ : any...) : int

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
@extern
public func swprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, _ : any...) : int

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
@extern
public func snwprintf_s(s : *mut wchar_t, n : rsize_t, format : *wchar_t, _ : any...) : int

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
@extern
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
@extern
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
@extern
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
@extern
public func vwprintf_s(format : *wchar_t, vlist : va_list) : int

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
@extern
public func vfwprintf_s(stream : *mut FILE, format : *wchar_t, vlist : va_list) : int

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
@extern
public func vswprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, vlist : va_list) : int

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
@extern
public func vsnwprintf_s(buffer : *mut wchar_t, bufsz : rsize_t, format : *wchar_t, vlist : va_list) : int

/**
 * If ps is not a null pointer, the mbsinit function determines whether the pointed-to mbstate_t object describes the initial conversion state.
 * @param ps	-	pointer to the mbstate_t object to examine
 * @return ​0​ if ps is not a null pointer and does not represent the initial conversion state, nonzero value otherwise.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbsinit
 */
@extern
public func mbsinit(ps : *mbstate_t) : int

/**
 * Widens a single-byte character c (reinterpreted as unsigned char) to its wide character equivalent.
 * Most multibyte character encodings use single-byte codes to represent the characters from the ASCII character set. This function may be used to convert such characters to wchar_t.
 * @param c	-	single-byte character to widen
 * @return WEOF if c is EOF
 *         wide character representation of c if (unsigned char)c is a valid single-byte character in the initial shift state, WEOF otherwise.
 * @see https://en.cppreference.com/w/c/string/multibyte/btowc
 */
@extern
public func btowc(c : int) : wint_t

/**
 * Narrows a wide character c if its multibyte character equivalent in the initial shift state is a single byte.
 * This is typically possible for the characters from the ASCII character set, since most multibyte encodings (such as UTF-8) use single bytes to encode those characters.
 * @param c	-	wide character to narrow
 * @return EOF if c does not represent a multibyte character with length 1 in initial shift state.
 *         otherwise, the single-byte representation of c as unsigned char converted to int
 * @see https://en.cppreference.com/w/c/string/multibyte/wctob
 */
@extern
public func wctob(c : wint_t) : int

/**
 * Determines the size, in bytes, of the representation of a multibyte character.
 * This function is equivalent to the call mbrtowc(NULL, s, n, ps ? ps : &internal) for some hidden object internal of type mbstate_t, except that the expression ps is evaluated only once.
 * @param s	-	pointer to an element of a multibyte character string
 * @param n	-	limit on the number of bytes in s that can be examined
 * @param ps	-	pointer to the variable holding the conversion state
 * @return The first of the following that applies:
 *         0 if the next n or fewer bytes complete the null character or if s is a null pointer. Both cases reset the conversion state.
 *         the number of bytes [1...n] that complete a valid multibyte character
 *         (size_t)-2 if the next n bytes are part of a possibly valid multibyte character, which is still incomplete after examining all n bytes
 *         (size_t)-1 if encoding error occurs. The value of errno is EILSEQ; the conversion state is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbrlen
 */
@extern
public func mbrlen(s : *char, n : size_t, ps : *mut mbstate_t) : size_t

/**
 * Determines the size, in bytes, of the representation of a multibyte character.
 * This function is equivalent to the call mbrtowc(NULL, s, n, ps ? ps : &internal) for some hidden object internal of type mbstate_t, except that the expression ps is evaluated only once.
 * @param s	-	pointer to an element of a multibyte character string
 * @param n	-	limit on the number of bytes in s that can be examined
 * @param ps	-	pointer to the variable holding the conversion state
 * @return The first of the following that applies:
 *         0 if the next n or fewer bytes complete the null character or if s is a null pointer. Both cases reset the conversion state.
 *         the number of bytes [1...n] that complete a valid multibyte character
 *         (size_t)-2 if the next n bytes are part of a possibly valid multibyte character, which is still incomplete after examining all n bytes
 *         (size_t)-1 if encoding error occurs. The value of errno is EILSEQ; the conversion state is unspecified.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbrlen
 */
@extern
public func mbrtowc(pwc : *mut wchar_t, s : *char, n : size_t, ps : *mut mbstate_t) : size_t

/**
 * If s is not a null pointer, the function determines the number of bytes necessary to store the multibyte character representation of wc (including any shift sequences, and taking into account the current multibyte conversion state *ps), and stores the multibyte character representation in the character array whose first element is pointed to by s, updating *ps as necessary. At most MB_CUR_MAX bytes can be written by this function.
 * If s is a null pointer, the call is equivalent to wcrtomb(buf, L'\0', ps) for some internal buffer buf.
 * If wc is the null wide character L'\0', a null byte is stored, preceded by any shift sequence necessary to restore the initial shift state and the conversion state parameter *ps is updated to represent the initial shift state.
 * If the environment macro __STDC_ISO_10646__ is defined, the values of type wchar_t are the same as the short identifiers of the characters in the Unicode required set (typically UTF-32 encoding); otherwise, it is implementation-defined. In any case, the multibyte character encoding used by this function is specified by the currently active C locale.
 * @param s	-	pointer to narrow character array where the multibyte character will be stored
 * @param wc	-	the wide character to convert
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @param ssz	-	max number of bytes to write (the size of the buffer s)
 * @param retval	-	pointer to an out-parameter where the result (number of bytes in the multibyte string including any shift sequences) will be stored
 * @return On success, returns the number of bytes (including any shift sequences) written to the character array whose first element is pointed to by s.
 *         On failure (if wc is not a valid wide character), returns (size_t)-1, stores EILSEQ in errno, and leaves *ps in unspecified state.
 * @see https://en.cppreference.com/w/c/string/multibyte/wcrtomb
 */
@extern
public func wcrtomb(s : *mut char, wc : wchar_t, ps : *mut mbstate_t) : size_t

/**
 * Same as (1), except that
 *   if s is a null pointer, the call is equivalent to wcrtomb_s(&retval, buf, sizeof buf, L'\0', ps) with internal variables retval and buf (whose size is greater than MB_CUR_MAX)
 *   the result is returned in the out-parameter retval
 *   the following errors are detected at runtime and call the currently installed constraint handler function:
 *          retval or ps is a null pointer.
 *          ssz is zero or greater than RSIZE_MAX (unless s is null)
 *          ssz is less than the number of bytes that would be written (unless s is null)
 *          s is a null pointer but ssz is not zero
 *  As with all bounds-checked functions, wcrtomb_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param s	-	pointer to narrow character array where the multibyte character will be stored
 * @param wc	-	the wide character to convert
 * @param ps	-	pointer to the conversion state object used when interpreting the multibyte string
 * @param ssz	-	max number of bytes to write (the size of the buffer s)
 * @param retval	-	pointer to an out-parameter where the result (number of bytes in the multibyte string including any shift sequences) will be stored
 * @return Returns zero on success and non-zero on failure, in which case, s[0] is set to '\0' (unless s is null or ssz is zero or greater than RSIZE_MAX) and *retval is set to (size_t)-1 (unless retval is null)
 * @see https://en.cppreference.com/w/c/string/multibyte/wcrtomb
 */
@extern
public func wcrtomb_s(retval : *mut size_t, s : *mut char, ssz : rsize_t, wc : wchar_t, ps : *mut mbstate_t) : errno_t

/**
 * Converts a null-terminated multibyte character sequence, which begins in the conversion state described by *ps, from the array whose first element is pointed to by *src to its wide character representation. If dst is not null, converted characters are stored in the successive elements of the wchar_t array pointed to by dst. No more than len wide characters are written to the destination array. Each multibyte character is converted as if by a call to mbrtowc. The conversion stops if:
 *      The multibyte null character was converted and stored. *src is set to null pointer value and *ps represents the initial shift state.
 *      An invalid multibyte character (according to the current C locale) was encountered. *src is set to point at the beginning of the first unconverted multibyte character.
 *      the next wide character to be stored would exceed len. *src is set to point at the beginning of the first unconverted multibyte character. This condition is not checked if dst is a null pointer.
 * @param dst	-	pointer to wide character array where the results will be stored
 * @param src	-	pointer to pointer to the first element of a null-terminated multibyte string
 * @param len	-	number of wide characters available in the array pointed to by dst
 * @param ps	-	pointer to the conversion state object
 * @param dstsz	-	max number of wide characters that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return On success, returns the number of wide characters, excluding the terminating L'\0', written to the character array. If dst is a null pointer, returns the number of wide characters that would have been written given unlimited length. On conversion error (if invalid multibyte character was encountered), returns (size_t)-1, stores EILSEQ in errno, and leaves *ps in unspecified state.
 * @see https://en.cppreference.com/w/c/string/multibyte/mbsrtowcs
 */
@extern
public func mbsrtowcs(dst : *mut wchar_t, src : **mut char, len : size_t, ps : *mut mbstate_t) : size_t

/**
 * Same as (1), except that
 *  the function returns its result as an out-parameter retval
 *  if no null character was written to dst after len wide characters were written, then L'\0' is stored in dst[len], which means len+1 total wide characters are written
 *  the function clobbers the destination array from the terminating null and until dstsz
 *  If src and dst overlap, the behavior is unspecified.
 *  the following errors are detected at runtime and call the currently installed constraint handler function:
 *      retval, ps, src, or *src is a null pointer
 *      dstsz or len is greater than RSIZE_MAX/sizeof(wchar_t) (unless dst is null)
 *      dstsz is not zero (unless dst is null)
 *      There is no null character in the first dstsz multibyte characters in the *src array and len is greater than dstsz (unless dst is null)
 *  As with all bounds-checked functions, mbsrtowcs_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dst	-	pointer to wide character array where the results will be stored
 * @param src	-	pointer to pointer to the first element of a null-terminated multibyte string
 * @param len	-	number of wide characters available in the array pointed to by dst
 * @param ps	-	pointer to the conversion state object
 * @param dstsz	-	max number of wide characters that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return zero on success (in which case the number of wide characters excluding terminating zero that were, or would be written to dst, is stored in *retval), non-sero on error. In case of a runtime constraint violation, stores (size_t)-1 in *retval (unless retval is null) and sets dst[0] to L'\0' (unless dst is null or dstmax is zero or greater than RSIZE_MAX)
 * @see https://en.cppreference.com/w/c/string/multibyte/mbsrtowcs
 */
@extern
public func mbsrtowcs_s(retval : *mut size_t, dst : *mut wchar_t, dstsz : rsize_t, src : **char, len : rsize_t, ps : *mut mbstate_t) : errno_t

/**
 * Converts a sequence of wide characters from the array whose first element is pointed to by *src to its narrow multibyte representation that begins in the conversion state described by *ps. If dst is not null, converted characters are stored in the successive elements of the char array pointed to by dst. No more than len bytes are written to the destination array. Each character is converted as if by a call to wcrtomb. The conversion stops if:
 *      The null character L'\0' was converted and stored. The bytes stored in this case are the unshift sequence (if necessary) followed by '\0', *src is set to null pointer value and *ps represents the initial shift state.
 *      A wchar_t was found that does not correspond to a valid character in the current C locale. *src is set to point at the first unconverted wide character.
 *      the next multibyte character to be stored would exceed len. *src is set to point at the first unconverted wide character. This condition is not checked if dst is a null pointer.
 * @param dst	-	pointer to narrow character array where the multibyte characters will be stored
 * @param src	-	pointer to pointer to the first element of a null-terminated wide string
 * @param len	-	number of bytes available in the array pointed to by dst
 * @param ps	-	pointer to the conversion state object
 * @param dstsz	-	max number of bytes that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return On success, returns the number of bytes (including any shift sequences, but excluding the terminating '\0') written to the character array whose first element is pointed to by dst. If dst is a null pointer, returns the number of bytes that would have been written. On conversion error (if invalid wide character was encountered), returns (size_t)-1, stores EILSEQ in errno, and leaves *ps in unspecified state.
 * @see https://en.cppreference.com/w/c/string/multibyte/wcsrtombs
 */
@extern
public func wcsrtombs(dst : *mut char, src : **wchar_t, len : size_t, ps : *mbstate_t) : size_t

/**
 * Same as (1), except that
 *  the function returns its result as an out-parameter retval
 *  if the conversion stops without writing a null character, the function will store '\0' in the next byte in dst, which may be dst[len] or dst[dstsz], whichever comes first (meaning up to len+1/dstsz+1 total bytes may be written). In this case, there may be no unshift sequence written before the terminating null.
 *  the function clobbers the destination array from the terminating null and until dstsz
 *  If src and dst overlap, the behavior is unspecified.
 *  the following errors are detected at runtime and call the currently installed constraint handler function:
 *      retval, ps, src, or *src is a null pointer
 *      dstsz or len is greater than RSIZE_MAX (unless dst is null)
 *      dstsz is not zero (unless dst is null)
 *      len is greater than dstsz and the conversion does not encounter null or encoding error in the src array by the time dstsz is reached (unless dst is null)
 *  As with all bounds-checked functions, wcsrtombs_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dst	-	pointer to narrow character array where the multibyte characters will be stored
 * @param src	-	pointer to pointer to the first element of a null-terminated wide string
 * @param len	-	number of bytes available in the array pointed to by dst
 * @param ps	-	pointer to the conversion state object
 * @param dstsz	-	max number of bytes that will be written (size of the dst array)
 * @param retval	-	pointer to a size_t object where the result will be stored
 * @return Returns zero on success (in which case the number of bytes excluding terminating zero that were, or would be written to dst, is stored in *retval), non-zero on error. In case of a runtime constraint violation, stores (size_t)-1 in *retval (unless retval is null) and sets dst[0] to '\0' (unless dst is null or dstmax is zero or greater than RSIZE_MAX)
 * @see https://en.cppreference.com/w/c/string/multibyte/wcsrtombs
 */
@extern
public func wcsrtombs_s(retval : *mut size_t, dst : *mut char, dstsz : rsize_t, src : **wchar_t, len : rsize_t, ps : *mut mbstate_t) : errno_t

/**
 * Converts the date and time information from a given calendar time time to a null-terminated wide character string str according to format string format. Up to count bytes are written.
 * @param str	-	pointer to the first element of the wchar_t array for output
 * @param count	-	maximum number of wide characters to write
 * @param format	-	pointer to a null-terminated wide character string specifying the format of conversion
 * @return Number of wide characters written into the wide character array pointed to by str not including the terminating L'\0' on success. If count was reached before the entire string could be stored, ​0​ is returned and the contents are undefined.
 * @see https://en.cppreference.com/w/c/chrono/wcsftime
 */
@extern
public func wcsftime(str : *mut wchar_t, count : size_t, format : *wchar_t, time : *tm) : size_t

/**
 * Interprets an integer value in a wide string pointed to by str.
 * Discards any whitespace characters (as identified by calling iswspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions sets the pointer pointed to by str_end to point to the wide character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to wide character
 * @param base	-	base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstol
 */
@extern
public func wcstol(str : *wchar_t, str_end : **mut wchar_t, base : int) : long

/**
 * Interprets an integer value in a wide string pointed to by str.
 * Discards any whitespace characters (as identified by calling iswspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) integer number representation and converts them to an integer value. The valid integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type.
 * The functions sets the pointer pointed to by str_end to point to the wide character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to wide character
 * @param base	-	base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and LONG_MAX, LONG_MIN, LLONG_MAX or LLONG_MIN is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstol
 */
@extern
public func wcstoll(str : *wchar_t, str_end : **mut wchar_t, base : int) : bigint

/**
 * Interprets an unsigned integer value in a wide string pointed to by str.
 * Discards any whitespace characters (as identified by calling iswspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) unsigned integer number representation and converts them to an integer value. The valid unsigned integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type, which applies unsigned integer wraparound rules.
 * The functions sets the pointer pointed to by str_end to point to the wide character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to a wide character.
 * @param base	-	base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and ULONG_MAX or ULLONG_MAX is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstoul
 */
@extern
public func wcstoul(str : *wchar_t, str_end : **mut wchar_t, base : int) : ulong

/**
 * Interprets an unsigned integer value in a wide string pointed to by str.
 * Discards any whitespace characters (as identified by calling iswspace) until the first non-whitespace character is found, then takes as many characters as possible to form a valid base-n (where n=base) unsigned integer number representation and converts them to an integer value. The valid unsigned integer value consists of the following parts:
 *      (optional) plus or minus sign
 *      (optional) prefix (0) indicating octal base (applies only when the base is 8 or ​0​)
 *      (optional) prefix (0x or 0X) indicating hexadecimal base (applies only when the base is 16 or ​0​)
 *      a sequence of digits
 * The set of valid values for base is {0,2,3,...,36}. The set of valid digits for base-2 integers is {0,1}, for base-3 integers is {0,1,2}, and so on. For bases larger than 10, valid digits include alphabetic characters, starting from Aa for base-11 integer, to Zz for base-36 integer. The case of the characters is ignored.
 * Additional numeric formats may be accepted by the currently installed C locale.
 * If the value of base is ​0​, the numeric base is auto-detected: if the prefix is 0, the base is octal, if the prefix is 0x or 0X, the base is hexadecimal, otherwise the base is decimal.
 * If the minus sign was part of the input sequence, the numeric value calculated from the sequence of digits is negated as if by unary minus in the result type, which applies unsigned integer wraparound rules.
 * The functions sets the pointer pointed to by str_end to point to the wide character past the last character interpreted. If str_end is a null pointer, it is ignored.
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to a wide character.
 * @param base	-	base of the interpreted integer value
 * @return Integer value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and ULONG_MAX or ULLONG_MAX is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstoul
 */
@extern
public func wcstoull(str : *wchar_t, str_end : **mut wchar_t, base : int) : ubigint

/**
 * Interprets a floating-point value in a wide string pointed to by str.
 * Function discards any whitespace characters (as determined by iswspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
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
 * any other expression that may be accepted by the currently installed C locale
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to a wide character.
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstof
 */
@extern
public func wcstof(str : *wchar_t, str_end : **mut wchar_t) : float

/**
 * Interprets a floating-point value in a wide string pointed to by str.
 * Function discards any whitespace characters (as determined by iswspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
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
 * any other expression that may be accepted by the currently installed C locale
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to a wide character.
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstof
 */
@extern
public func wcstod(str : *wchar_t, str_end : **mut wchar_t) : double

/**
 * Interprets a floating-point value in a wide string pointed to by str.
 * Function discards any whitespace characters (as determined by iswspace) until first non-whitespace character is found. Then it takes as many characters as possible to form a valid floating-point representation and converts them to a floating-point value. The valid floating-point value can be one of the following:
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
 * any other expression that may be accepted by the currently installed C locale
 * @param str	-	pointer to the null-terminated wide string to be interpreted
 * @param str_end	-	pointer to a pointer to a wide character.
 * @return Floating-point value corresponding to the contents of str on success. If the converted value falls out of range of corresponding return type, range error occurs and HUGE_VAL, HUGE_VALF or HUGE_VALL is returned. If no conversion can be performed, ​0​ is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcstof
 */
@extern
public func wcstold(str : *wchar_t, str_end : **mut wchar_t) : longdouble

/**
 * Copies the wide string pointed to by src (including the terminating null wide character) to wide character array pointed to by dest. The behavior is undefined if the dest array is not large enough. The behavior is undefined if the strings overlap.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/wide/wcscpy
 */
@extern
public func wcscpy(dest : *mut wchar_t, src : *wchar_t) : *mut wchar_t

/**
 * Same as (1), except that it may clobber the rest of the destination array with unspecified values and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz is zero or greater than RSIZE_MAX / sizeof(wchar_t)
 *      destsz is less or equal wcsnlen_s(src, destsz), in other words, truncation would occur
 *      overlap would occur between the source and the destination strings
 *  As with all bounds-checked functions, wcscpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes L'\0' to dest[0] (unless dest is a null pointer or destsz is zero or greater than RMAX_SIZE / sizeof(wchar_t)).
 * @see https://en.cppreference.com/w/c/string/wide/wcscpy
 */
@extern
public func wcscpy_s(dest : *wchar_t, destsz : rsize_t, src : *wchar_t) : errno_t

/**
 * Copies at most count characters of the wide string pointed to by src (including the terminating null wide character) to wide character array pointed to by dest.
 * If count is reached before the entire string src was copied, the resulting wide character array is not null-terminated.
 * If, after copying the terminating null wide character from src, count is not reached, additional null wide characters are written to dest until the total of count characters have been written.
 * If the strings overlap, the behavior is undefined.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide string to copy from
 * @param count	-	maximum number of wide characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/wide/wcsncpy
 */
@extern
public func wcsncpy(dest : *mut wchar_t, src : *wchar_t, count : size_t) : *mut wchar_t

/**
 * Same as (1), except that the function does not continue writing zeroes into the destination array to pad up to count, it stops after writing the terminating null character (if there was no null in the source, it writes one at dest[count] and then stops). Also, the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz or count is zero or greater than RSIZE_MAX/sizeof(wchar_t)
 *      count is greater or equal destsz, but destsz is less or equal wcsnlen_s(src, count), in other words, truncation would occur
 *      overlap would occur between the source and the destination strings
 *  As with all bounds-checked functions, wcsncpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide string to copy from
 * @param count	-	maximum number of wide characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes L'\0' to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX/sizeof(wchar_t)) and may clobber the rest of the destination array with unspecified values.
 * @see https://en.cppreference.com/w/c/string/wide/wcsncpy
 */
@extern
public func wcsncpy_s(dest : *mut wchar_t, destsz : rsize_t, src : *wchar_t, count : rsize_t) : errno_t

/**
 * Appends a copy of the wide string pointed to by src to the end of the wide string pointed to by dest. The wide character src[0] replaces the null terminator at the end of dest. The resulting wide string is null-terminated. The behavior is undefined if the destination array is not large enough for the contents of both str and dest and the terminating null wide character. The behavior is undefined if the strings overlap.
 * @param dest	-	pointer to the null-terminated wide string to append to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes L'\0' to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX/sizeof(wchar_t)).
 * @see https://en.cppreference.com/w/c/string/wide/wcscat
 */
@extern
public func wcscat(dest : *mut wchar_t, src : *wchar_t) : *mut wchar_t

/**
 * Same as (1), except that it may clobber the rest of the destination array (from the last character written to destsz) with unspecified values and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz is zero or greater than RSIZE_MAX/sizeof(wchar_t)
 *      there is no null terminator in the first destsz wide characters of dest
 *      truncation would occur (the available space at the end of dest would not fit every wide character, including the null terminator, of src)
 *      overlap would occur between the source and the destination strings
 *  As with all bounds-checked functions, wcscat_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the null-terminated wide string to append to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param destsz	-	maximum number of characters to write, typically the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes L'\0' to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX/sizeof(wchar_t)).
 * @see https://en.cppreference.com/w/c/string/wide/wcscat
 */
@extern
public func wcscat_s(dest : *mut wchar_t, destsz : rsize_t, src : *wchar_t) : errno_t

/**
 * Appends at most count wide characters from the wide string pointed to by src, stopping if the null terminator is copied, to the end of the character string pointed to by dest. The wide character src[0] replaces the null terminator at the end of dest. The null terminator is always appended in the end (so the maximum number of wide characters the function may write is count+1).
 * The behavior is undefined if the destination array is not large enough for the contents of both str and dest and the terminating null wide character.
 * The behavior is undefined if the strings overlap.
 * @param dest	-	pointer to the null-terminated wide string to append to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param count	-	maximum number of wide characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns a copy of dest
 */
@extern
public func wcsncat(dest : *mut wchar_t, src : *wchar_t, count : size_t) : *mut wchar_t

/**
 * Same as (1), except that this function may clobber the remainder of the destination array (from the last wide character written to destsz) and that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz or count is zero or greater than RSIZE_MAX/sizeof(wchar_t)
 *      there is no null wide character in the first destsz wide characters of dest
 *      truncation would occur: count or the length of src, whichever is less, exceeds the space available between the null terminator of dest and destsz.
 *      overlap would occur between the source and the destination strings
 * As with all bounds-checked functions, wcsncat_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the null-terminated wide string to append to
 * @param src	-	pointer to the null-terminated wide string to copy from
 * @param count	-	maximum number of wide characters to copy
 * @param destsz	-	the size of the destination buffer
 * @return returns zero on success, returns non-zero on error. Also, on error, writes L'\0' to dest[0] (unless dest is a null pointer or destsz is zero or greater than RSIZE_MAX/sizeof(wchar_t)).
 * @see https://en.cppreference.com/w/c/string/wide/wcsncat
 */
@extern
public func wcsncat_s(dest : *wchar_t, destsz : rsize_t, src : *wchar_t, count : rsize_t) : errno_t

/**
 * Transforms the null-terminated wide string pointed to by src into the implementation-defined form such that comparing two transformed strings with wcscmp gives the same result as comparing the original strings with wcscoll, in the current C locale.
 * The first count characters of the transformed string are written to destination, including the terminating null character, and the length of the full transformed string is returned, excluding the terminating null character.
 * If count is ​0​, then dest is allowed to be a null pointer.
 * @param dest	-	pointer to the first element of a wide null-terminated string to write the transformed string to
 * @param src	-	pointer to the null-terminated wide character string to transform
 * @param count	-	maximum number of characters to output
 * @return The length of the transformed wide string, not including the terminating null-character.
 * @see https://en.cppreference.com/w/c/string/wide/wcsxfrm
 */
@extern
public func wcsxfrm(dest : *mut wchar_t, src : *wchar_t, count : size_t) : size_t

/**
 * Returns the length of a wide string, that is the number of non-null wide characters that precede the terminating null wide character.
 * @param str	-	pointer to the null-terminated wide string to be examined
 * @param strsz	-	maximum number of wide characters to examine
 * @return The length of the null-terminated wide string str.
 * @see https://en.cppreference.com/w/c/string/wide/wcslen
 */
@extern
public func wcslen(str : *wchar_t) : size_t

/**
 * Same as (1), except that the function returns zero if str is a null pointer and returns strsz if the null wide character was not found in the first strsz wide characters of src
 * As with all bounds-checked functions, wcslen_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>..
 * @param str	-	pointer to the null-terminated wide string to be examined
 * @param strsz	-	maximum number of wide characters to examine
 * @return The length of the null-terminated wide string str on success, zero if str is a null pointer, strsz if the null wide character was not found.
 * @see https://en.cppreference.com/w/c/string/wide/wcslen
 */
@extern
public func wcsnlen_s(str : *wchar_t, strsz : size_t) : size_t

/**
 * Compares two null-terminated wide strings lexicographically.
 * The sign of the result is the sign of the difference between the values of the first pair of wide characters that differ in the strings being compared.
 * The behavior is undefined if lhs or rhs are not pointers to null-terminated wide strings.
 * @param lhs, rhs	-	pointers to the null-terminated wide strings to compare
 * @return Negative value if lhs appears before rhs in lexicographical order.
 *         Zero if lhs and rhs compare equal.
 *         Positive value if lhs appears after rhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/wide/wcscmp
 */
@extern
public func wcscmp(lhs : *wchar_t, rhs : *wchar_t) : int

/**
 * Compares at most count wide characters of two null-terminated wide strings. The comparison is done lexicographically.
 * The sign of the result is the sign of the difference between the values of the first pair of wide characters that differ in the strings being compared.
 * The behavior is undefined if lhs or rhs are not pointers to null-terminated strings.
 * @param lhs, rhs	-	pointers to the null-terminated wide strings to compare
 * @param count	-	maximum number of characters to compare
 * @return Negative value if lhs appears before rhs in lexicographical order.
 *         Zero if lhs and rhs compare equal.
 *         Positive value if lhs appears after rhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/wide/wcsncmp
 */
@extern
public func wcsncmp(lhs : *wchar_t, rhs : *wchar_t, count : size_t) : int

/**
 * Compares two null-terminated wide strings according to the collation order defined by the LC_COLLATE category of the currently installed locale.
 * @param lhs, rhs	-	pointers to the null-terminated wide strings to compare
 * @return Negative value if lhs is less than (precedes) rhs.
 *          0 if lhs is equal to rhs.
 *          Positive value if lhs is greater than (follows) rhs.
 * @see https://en.cppreference.com/w/c/string/wide/wcscoll
 */
@extern
public func wcscoll(lhs : *wchar_t, rhs : *wchar_t) : int

/**
 * Finds the first occurrence of the wide character ch in the wide string pointed to by str.
 * @param str	-	pointer to the null-terminated wide string to be analyzed
 * @param ch	-	wide character to search for
 * @return Pointer to the found character in str, or a null pointer if no such character is found.
 * @see https://en.cppreference.com/w/c/string/wide/wcschr
 */
@extern
public func wcschr(str : *wchar_t, ch : wchar_t) : *mut wchar_t

/**
 * TODO macro
 *   //QWchar_t// *wcschr( //QWchar_t// *str, wchar_t ch );
 * Type-generic function equivalent to (1). Let T be an unqualified wide character object type.
 *      If str is of type const T*, the return type is const wchar_t*.
 *      Otherwise, if str is of type T*, the return type is wchar_t*.
 *      Otherwise, the behavior is undefined.
 * If a macro definition of each of these generic functions is suppressed to access an actual function (e.g. if (wcschr) or a function pointer is used), the actual function declaration (1) becomes visible.
 * @see https://en.cppreference.com/w/c/string/wide/wcschr
 */

/**
 * Finds the last occurrence of the wide character ch in the wide string pointed to by str.
 * @param str	-	pointer to the null-terminated wide string to be analyzed
 * @param ch	-	wide character to search for
 * @return Pointer to the found character in str, or a null pointer if no such character is found.
 * @see https://en.cppreference.com/w/c/string/wide/wcsrchr
 */
@extern
public func wcsrchr(str : *wchar_t, ch : wchar_t) : *wchar_t

/**
 * TODO macro
 *   //QWchar_t// *wcsrchr( //QWchar_t// *str, wchar_t ch );
 * Type-generic function equivalent to (1). Let T be an unqualified wide character object type.
 * If str is of type const T*, the return type is const wchar_t*.
 * Otherwise, if str is of type T*, the return type is wchar_t*.
 * Otherwise, the behavior is undefined.
 * If a macro definition of each of these generic functions is suppressed to access an actual function (e.g. if (wcsrchr) or a function pointer is used), the actual function declaration (1) becomes visible.
 * @see https://en.cppreference.com/w/c/string/wide/wcsrchr
 */

/**
 * Returns the length of the maximum initial segment of the wide string pointed to by dest, that consists of only the characters found in wide string pointed to by src.
 * @param dest	-	pointer to the null-terminated wide string to be analyzed
 * @param src	-	pointer to the null-terminated wide string that contains the characters to search for
 * @return The length of the maximum initial segment that contains only characters from wide string pointed to by src
 * @see https://en.cppreference.com/w/c/string/wide/wcsspn
 */
@extern
public func wcsspn(dest : *wchar_t, src : *wchar_t) : size_t

/**
 * Returns the length of the maximum initial segment of the wide string pointed to by dest, that consists of only the characters not found in wide string pointed to by src.
 * @param dest	-	pointer to the null-terminated wide string to be analyzed
 * @param src	-	pointer to the null-terminated wide string that contains the characters to search for
 * @return The length of the maximum initial segment that contains only characters not found in the character string pointed to by src
 * @see https://en.cppreference.com/w/c/string/wide/wcscspn
 */
@extern
public func wcscspn(dest : *wchar_t, src : *wchar_t) : size_t

/**
 * Finds the first character in wide string pointed to by dest, that is also in wide string pointed to by str.
 * @param dest	-	pointer to the null-terminated wide string to be analyzed
 * @param src	-	pointer to the null-terminated wide string that contains the characters to search for
 * @return Pointer to the first character in dest, that is also in str, or a null pointer if no such character exists.
 * @see https://en.cppreference.com/w/c/string/wide/wcspbrk
 */
@extern
public func wcspbrk(dest : *wchar_t, str : *wchar_t) : *wchar_t

/**
 * TODO macro
 * //QWchar_t// *wcspbrk( //QWchar_t// *dest, const wchar_t *str );
 * @see https://en.cppreference.com/w/c/string/wide/wcspbrk
 */

/**
 * Finds the first occurrence of the wide string src in the wide string pointed to by dest. The terminating null characters are not compared.
 * @param dest	-	pointer to the null-terminated wide string to examine
 * @param src	-	pointer to the null-terminated wide string to search for
 * @return Pointer to the first character of the found substring in dest, or a null pointer if no such substring is found. If src points to an empty string, dest is returned.
 * @see https://en.cppreference.com/w/c/string/wide/wcsstr
 */
@extern
public func wcsstr(dest : *wchar_t, src : *wchar_t) : *wchar_t

/**
 * TODO macro
 * //QWchar_t// *wcsstr( //QWchar_t// *dest, const wchar_t *src );
 * @see https://en.cppreference.com/w/c/string/wide/wcsstr
 */

/**
 * Finds the next token in a null-terminated wide string pointed to by str. The separator characters are identified by null-terminated wide string pointed to by delim.
 * This function is designed to be called multiples times to obtain successive tokens from the same string.
 * If str != NULL, the call is treated as the first call to wcstok for this particular wide string. The function searches for the first wide character which is not contained in delim.
 * If no such wide character was found, there are no tokens in str at all, and the function returns a null pointer.
 * If such wide character was found, it is the beginning of the token. The function then searches from that point on for the first wide character that is contained in delim.
 * If no such wide character was found, str has only one token, and future calls to wcstok will return a null pointer
 * If such wide character was found, it is replaced by the null wide character L'\0' and the parser state (typically a pointer to the following wide character) is stored in the user-provided location *ptr.
 * The function then returns the pointer to the beginning of the token
 * If str == NULL, the call is treated as a subsequent call to wcstok: the function continues from where it left in the previous invocation with the same *ptr. The behavior is the same as if the pointer to the wide character that follows the last detected token is passed as str.
 * @param str	-	pointer to the null-terminated wide string to tokenize
 * @param delim	-	pointer to the null-terminated wide string identifying delimiters
 * @param ptr	-	pointer to an object of type wchar_t*, which is used by both wcstok and wcstok_s to store the internal state of the parser
 * @param strmax	-	pointer to an object which initially holds the size of str: wcstok_s stores the number of characters that remain to be examined
 * @return Returns pointer to the beginning of the next token or null pointer if there are no more tokens.
 * @see https://en.cppreference.com/w/c/string/wide/wcstok
 */
@extern
public func wcstok(str : *wchar_t, delim : *wchar_t, ptr : **mut wchar_t) : *wchar_t

/**
 * Copies exactly count successive wide characters from the wide character array pointed to by src to the wide character array pointed to by dest. If the objects overlap, the behavior is undefined. If count is zero, the function does nothing.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide character array to copy from
 * @param count	-	number of wide characters to copy
 * @param destsz	-	max number of wide characters to write (the size of the destination buffer)
 * @return returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/wide/wmemcpy
 */
@extern
public func wmemcpy(dest : *wchar_t, src : *wchar_t, count : size_t) : *wchar_t

/**
 * Same as (1), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz or count is greater than RSIZE_MAX/sizeof(wchar_t)
 *      count is greater than destsz (overflow would occur)
 *      overlap would occur between the source and the destination arrays
 * As with all bounds-checked functions, wmemcpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide character array to copy from
 * @param count	-	number of wide characters to copy
 * @param destsz	-	max number of wide characters to write (the size of the destination buffer)
 * @return returns zero on success, returns non-zero on error. Also, on error, fills the entire dst up to and not including dst+dstsz with null wide characters, L'\0' (unless dest is null or destsz is greater than RSIZE_MAX/sizeof(wchar_t))
 */
@extern
public func wmemcpy_s(dest : *wchar_t, destsz : rsize_t, src : *wchar_t, count : rsize_t) : errno_t

/**
 * Copies exactly count successive wide characters from the wide character array pointed to by src to the wide character array pointed to by dest. If count is zero, the function does nothing. The arrays may overlap: copying takes place as if the wide characters were copied to a temporary wide character array and then copied from the temporary array to dest.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide character array to copy from
 * @param destsz	-	max number of wide characters to write (the size of the destination buffer)
 * @param count	-	number of wide characters to copy
 * @return Returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/wide/wmemmove
 */
@extern
public func wmemmove(dest : *wchar_t, src : *wchar_t, count : size_t) : *wchar_t

/**
 * Same as (1), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      src or dest is a null pointer
 *      destsz or count is greater than RSIZE_MAX / sizeof(wchar_t)
 *      count is greater than destsz (overflow would occur)
 *  As with all bounds-checked functions, wmemcpy_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <wchar.h>.
 * @param dest	-	pointer to the wide character array to copy to
 * @param src	-	pointer to the wide character array to copy from
 * @param destsz	-	max number of wide characters to write (the size of the destination buffer)
 * @param count	-	number of wide characters to copy
 * @return Returns zero on success, returns non-zero on error. Also, on error, fills the entire dst up to and not including dst+dstsz with null wide characters, L'\0' (unless dest is null or destsz is greater than RSIZE_MAX/sizeof(wchar_t))
 * @see https://en.cppreference.com/w/c/string/wide/wmemmove
 */
@extern
public func wmemmove_s(dest : *wchar_t, destsz : rsize_t, src : *wchar_t, count : rsize_t) : errno_t

/**
 * Compares the first count wide characters of the wide character (or compatible integer type) arrays pointed to by lhs and rhs. The comparison is done lexicographically.
 * The sign of the result is the sign of the difference between the values of the first pair of wide characters that differ in the arrays being compared.
 * If count is zero, the function does nothing.
 * @param lhs, rhs	-	pointers to the wide character arrays to compare
 * @param count	-	number of wide characters to examine
 * @return Negative value if the value of the first differing wide character in lhs is less than the value of the corresponding wide character in rhs: lhs precedes rhs in lexicographical order.
 *         0 if all count wide characters of lhs and rhs are equal.
 *         Positive value if the value of the first differing wide character in lhs is greater than the value of the corresponding wide character in rhs: rhs precedes lhs in lexicographical order.
 * @see https://en.cppreference.com/w/c/string/wide/wmemcmp
 */
@extern
public func wmemcmp(lhs : *wchar_t, rhs : *wchar_t, count : size_t) : int

/**
 * Locates the first occurrence of wide character ch in the initial count wide characters of the wide character array or integer array of compatible type, pointed to by ptr.
 * @param ptr	-	pointer to the wide character array to be examined
 * @param ch	-	wide character to search for
 * @param count	-	number of wide characters to examine
 * @return Pointer to the location of the wide character, or a null pointer if no such character is found.
 * @see https://en.cppreference.com/w/c/string/wide/wmemchr
 */
@extern
public func wmemchr(ptr : *wchar_t, ch : wchar_t, count : size_t) : *wchar_t

/**
 * Type-generic function equivalent to (1). Let T be an unqualified wide character object type.
 * If ptr is of type const T*, the return type is const wchar_t*.
 * Otherwise, if ptr is of type T*, the return type is wchar_t*.
 * Otherwise, the behavior is undefined.
 * If a macro definition of each of these generic functions is suppressed to access an actual function (e.g. if (wmemchr) or a function pointer is used), the actual function declaration (1) becomes visible.
 * TODO macro
 * //QWchar_t// *wmemchr( //Qwchar_t// *ptr, wchar_t ch, size_t count );
 * @see https://en.cppreference.com/w/c/string/wide/wmemchr
 */

/**
 * Copies the wide character ch into each of the first count wide characters of the wide character array (or integer array of compatible type) pointed to by dest.
 * If overflow occurs, the behavior is undefined.
 * If count is zero, the function does nothing.
 * @param dest	-	pointer to the wide character array to fill
 * @param ch	-	fill wide character
 * @param count	-	number of wide characters to fill
 * @return Returns a copy of dest
 * @see https://en.cppreference.com/w/c/string/wide/wmemset
 */
@extern
public func wmemset(dest : *wchar_t, ch : wchar_t, count : size_t) : *wchar_t