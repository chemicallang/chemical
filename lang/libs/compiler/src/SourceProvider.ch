comptime const BufferCapacity = 1024

/**
 * provides access to the source code provided by the user
 */
@compiler.interface
public struct SourceProvider {

    var data_ptr : *char

    var data_len : size_t

    var data_end : *char

    var lineNumber : uint

    var lineCharacterNumber : uint

    /**
     * increment a single character forward
     */
    func increment(&self)

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    func readCharacter (&self) : char;

    /**
     * read a utf8 code point
     */
    func readCodePoint(provider : *mut SourceProvider) : u32;

    /**
     * peek a utf8 code point, get the length
     */
    func utf8_decode_peek(provider : *mut SourceProvider , out_len : &mut size_t) : u32;

    /**
     * increment a utf8 code point (that you peeked), giving its length
     */
    func incrementCodepoint(provider : *mut SourceProvider, cp : u32, len : size_t);

    /**
     * checks the stream is at the end
     * please also use both peek() == -1
     */
    func eof (&self) : bool

    /**
     * peaks the character to read
     */
    func peek (&self) : char

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    func increment_char (&self, c : char) : bool;

    /**
     * get zero-based current line number
     */
    func getLineNumber (&self) : uint;

    /**
     * get zero-based character number
     */
    func getLineCharNumber (&self) : uint

    /**
     * reads whitespaces, returns how many whitespaces were read
     */
    func readWhitespaces (&self) : uint;

    /**
     * @return whether there's a newline at current position
     */
    func hasNewLine (&self) : bool;

    /**
     * @return whether new line characters were read
     */
    func readNewLineChars (&self) : bool;

    /**
     * reads all whitespaces along with new lines
     */
    func readWhitespacesAndNewLines (&self) : void;

}

public func (provider : &SourceProvider) current_data() : *char {
    return provider.data_ptr;
}

// returns zero if stream ends before that
public func (provider : &SourceProvider) peek2() : char {
    const current = *provider.data_ptr;
    if(current == 0) return 0;
    const next = *(provider.data_ptr + 1)
    if(next == 0) return 0;
    return *(provider.data_ptr + 2)
}

public func (provider : &SourceProvider) getPosition() : Position {
    return Position {
        line : provider.lineNumber,
        character : provider.lineCharacterNumber
    }
}