/**
 * TODO FILE object is implementation defined
 * Each FILE object denotes a C stream.
 * C standard does not specify whether FILE is a complete object type. While it may be possible to copy a valid FILE, using a pointer to such a copy as an argument for an I/O function invokes unspecified behavior. In other words, FILE may be semantically non-copyable.
 * I/O streams can be used for both unformatted and formatted input and output. Furthermore, the functions that handle input and output can also be locale-sensitive, such that wide/multibyte conversions are performed as necessary.
 * @see https://en.cppreference.com/w/c/io/FILE
 */
struct FILE {

}

/**
 * TODO fpos_t is implementation defined
 * fpos_t is a non-array complete object type, can be used to store (by fgetpos) and restore (by fsetpos) the position and multibyte parser state (if any) for a C stream.
 * @see https://en.cppreference.com/w/c/io/fpos_t
 */
struct fpos_t {

}

/**
 * TODO streams are implementation defined
 * #define stdin   // implementation-defined
 * #define stdout  // implementation-defined
 * #define stderr  // implementation-defined
 * @see https://en.cppreference.com/w/c/io/std_streams
 */

/**
 * Opens a file indicated by filename and returns a pointer to the file stream associated with that file. mode is used to determine the file access mode.
 * @param filename (restrict)	-	file name to associate the file stream to
 * @param mode (restrict)	-	null-terminated character string determining file access mode
 * @return If successful, returns a pointer to the new file stream. The stream is fully buffered unless filename refers to an interactive device. On error, returns a null pointer. POSIX requires that errno be set in this case.
 * @see https://en.cppreference.com/w/c/io/fopen
 */
public func *fopen(filename : *char, mode : *char) : FILE

/**
 * Same as fopen, except that the pointer to the file stream is written to streamptr and the following errors are detected at runtime and call the currently installed constraint handler function:
 * streamptr is a null pointer
 * filename is a null pointer
 * mode is a null pointer
 * As with all bounds-checked functions, fopen_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param streamptr (restrict)	-	pointer to a pointer where the function stores the result (an out-parameter)
 * @param filename (restrict)	-	file name to associate the file stream to
 * @param mode (restrict)	-	null-terminated character string determining file access mode
 * @return If successful, returns zero and a pointer to the new file stream is written to *streamptr. On error, returns a non-zero error code and writes the null pointer to *streamptr (unless streamptr is a null pointer itself).
 * @see https://en.cppreference.com/w/c/io/fopen
 */
public func fopen_s(streamptr : **mut FILE, filename : *char, mode : *char) : errno_t

/**
 * First, attempts to close the file associated with stream, ignoring any errors. Then, if filename is not null, attempts to open the file specified by filename using mode as if by fopen, and associates that file with the file stream pointed to by stream. If filename is a null pointer, then the function attempts to reopen the file that is already associated with stream (it is implementation defined which mode changes are allowed in this case).
 * @param filename	-	file name to associate the file stream to
 * @param mode	-	null-terminated character string determining new file access mode
 * @param stream	-	the file stream to modify
 * @return A copy of the value of stream on success, null pointer on failure.
 * @see https://en.cppreference.com/w/c/io/freopen
 */
public func freopen(filename : *char, mode : *char, stream : *mut FILE) : *FILE

/**
 * Same as (1), except that mode is treated as in fopen_s and that the pointer to the file stream is written to newstreamptr and the following errors are detected at runtime and call the currently installed constraint handler function:
 * newstreamptr is a null pointer
 * stream is a null pointer
 * mode is a null pointer
 * As with all bounds-checked functions, freopen_s is only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param filename	-	file name to associate the file stream to
 * @param mode	-	null-terminated character string determining new file access mode
 * @param stream	-	the file stream to modify
 * @param newstreamptr	-	pointer to a pointer where the function stores the result (an out-parameter)
 * @return zero on success (and a copy of the value of stream is written to *newstreamptr, non-zero on error (and null pointer is written to *newstreamptr unless newstreamptr is itself a null pointer).
 * @see https://en.cppreference.com/w/c/io/freopen
 */
public func freopen_s(newstreamptr : **mut FILE, filename : *char, mode : *char, stream : *mut FILE) : errno_t

/**
 * Closes the given file stream. Any unwritten buffered data are flushed to the OS. Any unread buffered data are discarded.
 * Whether or not the operation succeeds, the stream is no longer associated with a file, and the buffer allocated by setbuf or setvbuf, if any, is also disassociated and deallocated if automatic allocation was used.
 * The behavior is undefined if the value of the pointer stream is used after fclose returns.
 * @param stream	-	the file stream to close
 * @return 0 on success, EOF otherwise
 * @see https://en.cppreference.com/w/c/io/fclose
 */
public func fclose(stream : *mut FILE) : int

/**
 * For output streams (and for update streams on which the last operation was output), writes any unwritten data from the stream's buffer to the associated output device.
 * For input streams (and for update streams on which the last operation was input), the behavior is undefined.
 * If stream is a null pointer, all open output streams are flushed, including the ones manipulated within library packages or otherwise not directly accessible to the program.
 * @param stream	-	the file stream to write out
 * @return Returns zero on success. Otherwise EOF is returned and the error indicator of the file stream is set.
 * @see https://en.cppreference.com/w/c/io/fflush
 */
public func fflush(stream : *mut FILE) : int

/**
 * Sets the internal buffer to use for stream operations. It should be at least BUFSIZ characters long.
 * If buffer is not null, equivalent to setvbuf(stream, buffer, _IOFBF, BUFSIZ).
 * If buffer is null, equivalent to setvbuf(stream, NULL, _IONBF, 0), which turns off buffering.
 * @param stream	-	the file stream to set the buffer to
 * @param buffer	-	pointer to a buffer for the stream to use. If a null pointer is supplied, the buffering is turned off
 * @see https://en.cppreference.com/w/c/io/setbuf
 */
public func setbuf(stream : *FILE, buffer : *char);

/**
 * TODO macro
 * #define BUFSIZ
 * @see https://en.cppreference.com/w/c/io/setbuf
 */

/**
 * Changes the buffering mode of the given file stream stream as indicated by the argument mode. In addition,
 * If buffer is a null pointer, resizes the internal buffer to size.
 * If buffer is not a null pointer, instructs the stream to use the user-provided buffer of size size beginning at buffer. The stream must be closed (with fclose) before the lifetime of the array pointed to by buffer ends. The contents of the array after a successful call to setvbuf are indeterminate and any attempt to use it is undefined behavior.
 * @param stream	-	the file stream to set the buffer to
 * @param buffer	-	pointer to a buffer for the stream to use or null pointer to change size and mode only
 * @param mode	-	buffering mode to use. It can be one of the following values:
 *      _IOFBF	full buffering
 *      _IOLBF	line buffering
 *      _IONBF	no buffering
 * @param size	-	size of the buffer
 * @return 0 on success or nonzero on failure.
 * @see https://en.cppreference.com/w/c/io/setvbuf
 */
public func setvbuf(stream : *mut FILE, buffer : *mut char, mode : int, size : size_t) : int

/**
 * Reads up to count objects into the array buffer from the given input stream stream as if by calling fgetc size times for each object, and storing the results, in the order obtained, into the successive positions of buffer, which is reinterpreted as an array of unsigned char. The file position indicator for the stream is advanced by the number of characters read.
 * If an error occurs, the resulting value of the file position indicator for the stream is indeterminate. If a partial element is read, its value is indeterminate.
 * @param buffer	-	pointer to the array where the read objects are stored
 * @param size	-	size of each object in bytes
 * @param count	-	the number of the objects to be read
 * @param stream	-	the stream to read
 * @return Number of objects read successfully, which may be less than count if an error or end-of-file condition occurs.
 *         If size or count is zero, fread returns zero and performs no other action.
 *         fread does not distinguish between end-of-file and error, and callers must use feof and ferror to determine which occurred.
 */
public func fread(buffer : *mut void, size : size_t, count : size_t, stream : *mut FILE) : size_t

/**
 * Writes count of objects from the given array buffer to the output stream stream. The objects are written as if by reinterpreting each object as an array of unsigned char and calling fputc size times for each object to write those unsigned chars into stream, in order. The file position indicator for the stream is advanced by the number of characters written.
 * If an error occurs, the resulting value of the file position indicator for the stream is indeterminate.
 * @param buffer	-	pointer to the first object in the array to be written
 * @param size	-	size of each object
 * @param count	-	the number of the objects to be written
 * @param stream	-	pointer to the output stream
 * @return The number of objects written successfully, which may be less than count if an error occurs.
 *         If size or count is zero, fwrite returns zero and performs no other action.
 * @see https://en.cppreference.com/w/c/io/fwrite
 */
public func fwrite(buffer : *void, size : size_t, count : size_t, stream : *mut FILE) : size_t

/**
 * Reads the next character from the given input stream.
 * @param stream	-	to read the character from
 * @return On success, returns the obtained character as an unsigned char converted to an int. On failure, returns EOF.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stream. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream.
 * @see https://en.cppreference.com/w/c/io/fgetc
 */
public func fgetc(stream : *mut FILE) : int

/**
 * Reads at most count - 1 characters from the given file stream and stores them in the character array pointed to by str. Parsing stops if a newline character is found (in which case str will contain that newline character) or if end-of-file occurs. If bytes are read and no errors occur, writes a null character at the position immediately after the last character written to str.
 * @param str	-	pointer to an element of a char array
 * @param count	-	maximum number of characters to write (typically the length of str)
 * @param stream	-	file stream to read the data from
 * @return str on success, null pointer on failure.
 *         If the end-of-file condition is encountered, sets the eof indicator on stream (see feof()). This is only a failure if it causes no bytes to be read, in which case a null pointer is returned and the contents of the array pointed to by str are not altered (i.e. the first byte is not overwritten with a null character).
 *         If the failure has been caused by some other error, sets the error indicator (see ferror()) on stream. The contents of the array pointed to by str are indeterminate (it may not even be null-terminated).
 * @see https://en.cppreference.com/w/c/io/fgets
 */
public func fgets(str : *mut char, count : int, stream : *mut FILE) : *mut char

/**
 * Writes a character ch to the given output stream stream. putc() may be implemented as a macro and evaluate stream more than once, so the corresponding argument should never be an expression with side effects.
 * Internally, the character is converted to unsigned char just before being written.
 * @param ch	-	character to be written
 * @param stream	-	output stream
 * @return On success, returns the written character.
 *         On failure, returns EOF and sets the error indicator (see ferror()) on stream.
 * @see https://en.cppreference.com/w/c/io/fputc
 */
public func fputc(ch : int, stream : *mut FILE) : int

/**
 * Writes a character ch to the given output stream stream. putc() may be implemented as a macro and evaluate stream more than once, so the corresponding argument should never be an expression with side effects.
 * Internally, the character is converted to unsigned char just before being written.
 * @param ch	-	character to be written
 * @param stream	-	output stream
 * @return On success, returns the written character.
 *         On failure, returns EOF and sets the error indicator (see ferror()) on stream.
 * @see https://en.cppreference.com/w/c/io/fputc
 */
public func putc(ch : int, stream : *mut FILE) : int

/**
 * Writes every character from the null-terminated string str to the output stream stream, as if by repeatedly executing fputc.
 * The terminating null character from str is not written.
 * @param str	-	null-terminated character string to be written
 * @param stream	-	output stream
 * @return On success, returns a non-negative value.
 *         On failure, returns EOF and sets the error indicator (see ferror()) on stream.
 * @see https://en.cppreference.com/w/c/io/fputs
 */
public func fputs(str : *char, stream : *mut FILE) : int

/**
 * Reads the next character from stdin.
 * Equivalent to getc(stdin).
 * @return The obtained character on success or EOF on failure.
 *         If the failure has been caused by end-of-file condition, additionally sets the eof indicator (see feof()) on stdin. If the failure has been caused by some other error, sets the error indicator (see ferror()) on stdin.
 * @see https://en.cppreference.com/w/c/io/getchar
 */
public func getchar() : int

/**
 * Writes a character ch to stdout. Internally, the character is converted to unsigned char just before being written.
 * Equivalent to putc(ch, stdout).
 * @param ch	-	character to be written
 * @return On success, returns the written character.
 *         On failure, returns EOF and sets the error indicator (see ferror()) on stdout.
 * @see https://en.cppreference.com/w/c/io/putchar
 */
public func putchar(ch : int) : int

/**
 * Writes every character from the null-terminated string str and one additional newline character '\n' to the output stream stdout, as if by repeatedly executing fputc.
 * The terminating null character from str is not written.
 * @param str	-	character string to be written
 * @return On success, returns a non-negative value.
 *         On failure, returns EOF and sets the error indicator (see ferror()) on stream.
 * @see https://en.cppreference.com/w/c/io/puts
 */
public func puts(str : *char) : int

/**
 * If ch does not equal EOF, pushes the character ch (reinterpreted as unsigned char) into the input buffer associated with the stream stream in such a manner that subsequent read operation from stream will retrieve that character. The external device associated with the stream is not modified.
 * Stream repositioning operations fseek, fsetpos, and rewind discard the effects of ungetc.
 * If ungetc is called more than once without an intervening read or repositioning, it may fail (in other words, a pushback buffer of size 1 is guaranteed, but any larger buffer is implementation-defined). If multiple successful ungetc were performed, read operations retrieve the pushed-back characters in reverse order of ungetc.
 * If ch equals EOF, the operation fails and the stream is not affected.
 * A successful call to ungetc clears the end of file status flag feof.
 * A successful call to ungetc on a binary stream decrements the stream position indicator by one (the behavior is indeterminate if the stream position indicator was zero).
 * A successful call to ungetc on a text stream modifies the stream position indicator in unspecified manner but guarantees that after all pushed-back characters are retrieved with a read operation, the stream position indicator is equal to its value before ungetc.
 * @param ch	-	character to be pushed into the input stream buffer
 * @param stream	-	file stream to put the character back to
 * @return On success ch is returned.
 *         On failure EOF is returned and the given stream remains unchanged.
 * @see https://en.cppreference.com/w/c/io/ungetc
 */
public func ungetc(ch : int, stream : *mut FILE) : int


/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * reads the data from stdin
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Number of receiving arguments successfully assigned (which may be zero in case a matching failure occurred before the first receiving argument was assigned), or EOF if input failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func scanf(format : *char, _ : ... ) : int

/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * reads the data from file stream
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Number of receiving arguments successfully assigned (which may be zero in case a matching failure occurred before the first receiving argument was assigned), or EOF if input failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func fscanf(stream : *mut FILE, format : *char, _ : ... ) : int

/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * reads the data from null-terminated character string buffer. Reaching the end of the string is equivalent to reaching the end-of-file condition for fscanf
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Number of receiving arguments successfully assigned (which may be zero in case a matching failure occurred before the first receiving argument was assigned), or EOF if input failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func sscanf(buffer : *char, format : *char, _ : ... ) : int

/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  any of the arguments of pointer type is a null pointer
 *  format, stream, or buffer is a null pointer
 *  the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *  optionally, any other detectable error, such as unknown conversion specifier
 *  As with all bounds-checked functions, scanf_s, fscanf_s, and sscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func scanf_s(format : *char, ...) : int

/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  any of the arguments of pointer type is a null pointer
 *  format, stream, or buffer is a null pointer
 *  the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *  optionally, any other detectable error, such as unknown conversion specifier
 *  As with all bounds-checked functions, scanf_s, fscanf_s, and sscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func fscanf_s(stream : *mut FILE, format : *char, _ : ...) : int

/**
 * Reads data from a variety of sources, interprets it according to format and stores the results into given locations.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  any of the arguments of pointer type is a null pointer
 *  format, stream, or buffer is a null pointer
 *  the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 *  optionally, any other detectable error, such as unknown conversion specifier
 *  As with all bounds-checked functions, scanf_s, fscanf_s, and sscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param ...	-	receiving arguments.
 * The format string consists of
 *  non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *  whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *  conversion specifications. Each conversion specification has the following format:
 *  introductory % character.
 *  (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *  (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *  (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *  conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/fscanf
 */
public func sscanf_s(buffer : *char, format : *char, _ : ...) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from stdin
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vscanf(format : *char, vlist : va_list ) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from file stream stream
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vfscanf(stream : *mut FILE, format : *char, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Reads the data from null-terminated character string buffer. Reaching the end of the string is equivalent to reaching the end-of-file condition for fscanf
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Number of receiving arguments successfully assigned, or EOF if read failure occurs before the first receiving argument was assigned.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vsscanf(buffer : *char, format : *char, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 * any of the arguments of pointer type is a null pointer
 * format, stream, or buffer is a null pointer
 * the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 * optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vscanf_s, vfscanf_s, and vsscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vscanf_s(format : *char, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 * any of the arguments of pointer type is a null pointer
 * format, stream, or buffer is a null pointer
 * the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 * optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vscanf_s, vfscanf_s, and vsscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vfscanf_s(stream : *mut FILE, format : *char, vlist : va_list) : int

/**
 * Reads data from the a variety of sources, interprets it according to format and stores the results into locations defined by vlist.
 * Same as (1-3), except that %c, %s, and %[ conversion specifiers each expect two arguments (the usual pointer and a value of type rsize_t indicating the size of the receiving array, which may be 1 when reading with a %c into a single char) and except that the following errors are detected at runtime and call the currently installed constraint handler function:
 * any of the arguments of pointer type is a null pointer
 * format, stream, or buffer is a null pointer
 * the number of characters that would be written by %c, %s, or %[, plus the terminating null character, would exceed the second (rsize_t) argument provided for each of those conversion specifiers
 * optionally, any other detectable error, such as unknown conversion specifier
 * As with all bounds-checked functions, vscanf_s, vfscanf_s, and vsscanf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	input file stream to read from
 * @param buffer	-	pointer to a null-terminated character string to read from
 * @param format	-	pointer to a null-terminated character string specifying how to read the input
 * @param vlist	-	variable argument list containing the receiving arguments.
 * The format string consists of
 *   non-whitespace multibyte characters except %: each such character in the format string consumes exactly one identical character from the input stream, or causes the function to fail if the next character on the stream does not compare equal.
 *   whitespace characters: any single whitespace character in the format string consumes all available consecutive whitespace characters from the input (determined as if by calling isspace in a loop). Note that there is no difference between "\n", " ", "\t\t", or other whitespace in the format string.
 *   conversion specifications. Each conversion specification has the following format:
 *   introductory % character.
 *   (optional) assignment-suppressing character *. If this option is present, the function does not assign the result of the conversion to any receiving argument.
 *   (optional) integer number (greater than zero) that specifies maximum field width, that is, the maximum number of characters that the function is allowed to consume when doing the conversion specified by the current conversion specification. Note that %s and %[ may lead to buffer overflow if the width is not provided.
 *   (optional) length modifier that specifies the size of the receiving argument, that is, the actual destination type. This affects the conversion accuracy and overflow rules. The default destination type is different for each conversion type (see table below).
 *   conversion format specifier.
 * @return Same as (1-3), except that EOF is also returned if there is a runtime constraint violation.
 * @see https://en.cppreference.com/w/c/io/vfscanf
 */
public func vsscanf_s(buffer : *char, format : *char, vlist : va_list) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Writes the results to the output stream stdout.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters transmitted to the output stream or negative value if an output error or an encoding error (for string and character conversion specifiers) occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func printf(format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Writes the results to the output stream stream.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters transmitted to the output stream or negative value if an output error or an encoding error (for string and character conversion specifiers) occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func fprintf(stream : *mut FILE, format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Writes the results to a character string buffer. The behavior is undefined if the string to be written (plus the terminating null character) exceeds the size of the array pointed to by buffer.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters written to buffer (not counting the terminating null character), or a negative value if an encoding error (for string and character conversion specifiers) occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func sprintf(buffer : *mut char, format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Writes the results to a character string buffer. At most bufsz - 1 characters are written. The resulting character string will be terminated with a null character, unless bufsz is zero. If bufsz is zero, nothing is written and buffer may be a null pointer, however the return value (number of bytes that would be written not including the null terminator) is still calculated and returned.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters (not including the terminating null character) which would have been written to buffer if bufsz was ignored, or a negative value if an encoding error (for string and character conversion specifiers) occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func snprintf(buffer : *mut char, bufsz : size_t, format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  the conversion specifier %n is present in format
 *  any of the arguments corresponding to %s is a null pointer
 *  stream or format or buffer is a null pointer
 *  bufsz is zero or greater than RSIZE_MAX
 *  encoding errors occur in any of string and character conversion specifiers
 *  (for sprintf_s only), the string to be stored in buffer (including the trailing null) would exceed bufsz.
 *  As with all bounds-checked functions, printf_s, fprintf_s, sprintf_s, and snprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters transmitted to the output stream or negative value if an output error, a runtime constraints violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func printf_s(format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  the conversion specifier %n is present in format
 *  any of the arguments corresponding to %s is a null pointer
 *  stream or format or buffer is a null pointer
 *  bufsz is zero or greater than RSIZE_MAX
 *  encoding errors occur in any of string and character conversion specifiers
 *  (for sprintf_s only), the string to be stored in buffer (including the trailing null) would exceed bufsz.
 *  As with all bounds-checked functions, printf_s, fprintf_s, sprintf_s, and snprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters transmitted to the output stream or negative value if an output error, a runtime constraints violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func fprintf_s(stream : *mut FILE, format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  the conversion specifier %n is present in format
 *  any of the arguments corresponding to %s is a null pointer
 *  stream or format or buffer is a null pointer
 *  bufsz is zero or greater than RSIZE_MAX
 *  encoding errors occur in any of string and character conversion specifiers
 *  (for sprintf_s only), the string to be stored in buffer (including the trailing null) would exceed bufsz.
 *  As with all bounds-checked functions, printf_s, fprintf_s, sprintf_s, and snprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters written to buffer, not counting the null character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX), or zero on runtime constraint violations, and negative value on encoding errors.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func sprintf_s(buffer : *mut char, bufsz : rsize_t, format : *char, _ : any...) : int

/**
 * Loads the data from the given locations, converts them to character string equivalents and writes the results to a variety of sinks/streams
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *  the conversion specifier %n is present in format
 *  any of the arguments corresponding to %s is a null pointer
 *  stream or format or buffer is a null pointer
 *  bufsz is zero or greater than RSIZE_MAX
 *  encoding errors occur in any of string and character conversion specifiers
 *  (for sprintf_s only), the string to be stored in buffer (including the trailing null) would exceed bufsz.
 *  As with all bounds-checked functions, printf_s, fprintf_s, sprintf_s, and snprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated byte string specifying how to interpret the data
 * @param ...	-	arguments specifying data to print. If any argument after default argument promotions is not the type expected by the corresponding conversion specifier, or if there are fewer arguments than required by format, the behavior is undefined. If there are more arguments than required by format, the extraneous arguments are evaluated and ignored.
 * @return The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
 *          introductory % character.
 *          (optional) one or more flags that modify the behavior of the conversion:
 *              -: the result of the conversion is left-justified within the field (by default it is right-justified).
 *              +: the sign of signed conversions is always prepended to the result of the conversion (by default the result is preceded by minus only when it is negative).
 *              space: if the result of a signed conversion does not start with a sign character, or is empty, space is prepended to the result. It is ignored if + flag is present.
 *              #: alternative form of the conversion is performed. See the table below for exact effects otherwise the behavior is undefined.
 *              0: for integer and floating-point number conversions, leading zeros are used to pad the field instead of space characters. For integer numbers it is ignored if the precision is explicitly specified. For other conversions using this flag results in undefined behavior. It is ignored if - flag is present.
 *          (optional) integer value or * that specifies minimum field width. The result is padded with space characters (by default), if required, on the left when right-justified, or on the right if left-justified. In the case when * is used, the width is specified by an additional argument of type int, which appears before the argument to be converted and the argument supplying precision if one is supplied. If the value of the argument is negative, it results with the - flag specified and positive field width (Note: This is the minimum width: The value is never truncated.).
 *          (optional) . followed by integer number or *, or neither that specifies precision of the conversion. In the case when * is used, the precision is specified by an additional argument of type int, which appears before the argument to be converted, but after the argument supplying minimum field width if one is supplied. If the value of this argument is negative, it is ignored. If neither a number nor * is used, the precision is taken as zero. See the table below for exact effects of precision.
 *          (optional) length modifier that specifies the size of the argument (in combination with the conversion format specifier, it specifies the type of the corresponding argument).
 *          conversion format specifier.
 * @return number of characters not including the terminating null character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX), which would have been written to buffer if bufsz was ignored, or a negative value if a runtime constraints violation or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/fprintf
 */
public func snprintf_s(buffer : *mut char, bufsz : rsize_t, format : *char, _ : any...) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Writes the results to stdout.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return The number of characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vprintf(format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Writes the results to a file stream stream
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return The number of characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vfprintf(stream : *FILE, format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Writes the results to a character string buffer.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return The number of characters written if successful or negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vsprintf(buffer : *mut char, format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Writes the results to a character string buffer. At most bufsz - 1 characters are written. The resulting character string will be terminated with a null character, unless bufsz is zero. If bufsz is zero, nothing is written and buffer may be a null pointer, however the return value (number of bytes that would be written not including the null terminator) is still calculated and returned.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return The number of characters written if successful or negative value if an error occurred. If the resulting string gets truncated due to buf_size limit, function returns the total number of characters (not including the terminating null-byte) which would have been written, if the limit was not imposed.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vsnprintf(buffer : *mut char, bufsz : size_t, format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vsprintf_s only), the string to be stored in buffer (including the trailing null)) would be exceed bufsz
 * As with all bounds-checked functions, vprintf_s, vfprintf_s, vsprintf_s, and vsnprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return number of characters transmitted to the output stream or negative value if an output error, a runtime constrants violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vprintf_s(format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vsprintf_s only), the string to be stored in buffer (including the trailing null)) would be exceed bufsz
 * As with all bounds-checked functions, vprintf_s, vfprintf_s, vsprintf_s, and vsnprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return number of characters transmitted to the output stream or negative value if an output error, a runtime constrants violation error, or an encoding error occurred.
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vfprintf_s(stream : *mut FILE, format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vsprintf_s only), the string to be stored in buffer (including the trailing null)) would be exceed bufsz
 * As with all bounds-checked functions, vprintf_s, vfprintf_s, vsprintf_s, and vsnprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return number of characters written to buffer, not counting the null character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX), or zero on runtime constraint violations, and negative value on encoding errors
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vsprintf_s(buffer : *mut char, bufsz : rsize_t, format : *char, vlist : va_list) : int

/**
 * Loads the data from the locations, defined by vlist, converts them to character string equivalents and writes the results to a variety of sinks.
 * Same as (1-4), except that the following errors are detected at runtime and call the currently installed constraint handler function:
 *      the conversion specifier %n is present in format
 *      any of the arguments corresponding to %s is a null pointer
 *      format or buffer is a null pointer
 *      bufsz is zero or greater than RSIZE_MAX
 *      encoding errors occur in any of string and character conversion specifiers
 *      (for vsprintf_s only), the string to be stored in buffer (including the trailing null)) would be exceed bufsz
 * As with all bounds-checked functions, vprintf_s, vfprintf_s, vsprintf_s, and vsnprintf_s are only guaranteed to be available if __STDC_LIB_EXT1__ is defined by the implementation and if the user defines __STDC_WANT_LIB_EXT1__ to the integer constant 1 before including <stdio.h>.
 * @param stream	-	output file stream to write to
 * @param buffer	-	pointer to a character string to write to
 * @param bufsz	-	up to bufsz - 1 characters may be written, plus the null terminator
 * @param format	-	pointer to a null-terminated character string specifying how to interpret the data
 * @param vlist	-	variable argument list containing the data to print.
 * The format string consists of ordinary byte characters (except %), which are copied unchanged into the output stream, and conversion specifications. Each conversion specification has the following format:
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
 * @return number of characters not including the terminating null character (which is always written as long as buffer is not a null pointer and bufsz is not zero and not greater than RSIZE_MAX), which would have been written to buffer if bufsz was ignored, or a negative value if a runtime constraints violation or an encoding error occurred
 * @see https://en.cppreference.com/w/c/io/vfprintf
 */
public func vsnprintf_s(buffer : *mut char, bufsz : rsize_t, format : *char, vlist : va_list) : int
