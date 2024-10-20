/**
 * If mode > 0, attempts to make stream wide-oriented. If mode < 0, attempts to make stream byte-oriented. If mode==0, only queries the current orientation of the stream.
 * If the orientation of the stream has already been decided (by executing output or by an earlier call to fwide), this function does nothing.
 * @param stream	-	pointer to the C I/O stream to modify or query
 * @param mode	-	integer value greater than zero to set the stream wide, less than zero to set the stream narrow, or zero to query only
 * @return An integer greater than zero if the stream is wide-oriented after this call, less than zero if the stream is byte-oriented after this call, and zero if the stream has no orientation.
 * @see https://en.cppreference.com/w/c/io/fwide
 */
public func fwide(stream : *mut FILE, mode : int) : int