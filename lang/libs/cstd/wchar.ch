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