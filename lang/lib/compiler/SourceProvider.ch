import "./Operation.ch"
import "@std/string.ch"

/**
 * provides access to the source code provided by the user
 */
@compiler.interface
struct SourceProvider {

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    func readCharacter (&self) : char;

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
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    func readUntil (&self, into : *string, stop : char);

    /**
     * if text is present at current pos in the stream, increments the stream with text.length()
     * @param text to increment
     * @param peek peeks only, doesn't increment
     * @return true if incremented by text length otherwise false
     */
    func increment (&self, text : *string, peek : bool) : bool;

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
     * will read everything to the given string
     *
     * will not stop if the stream doesn't end or there's a backslash before stopAt character
     * useful when reading a string token which must not stop at \"
     *
     * will also append the last stopAt character into value
     */
    func readEscaping (&self, value : *string, stopAt : char) : void;

    /**
     * reads all characters into a string until char occurs
     * @return the string that was found
     */
    func readAnything (&self, into : *string, until : char) : void;

    /**
     * reads a alphabetical string
     */
    func readAlpha (&self, into : *string) : void;

    /**
     * reads an unsigned integer as string, returns "" if no integer found
     */
    func readUnsignedInt (&self, into : *string) : void;

    /**
     * reads a number from the stream
     */
    func readNumber (&self, into : *string) : void;

    /**
     * reads a alphanumeric string
     */
    func readAlphaNum (&self, into : *string) : void;

    /**
     * reads a single identifier
     */
    func readIdentifier (&self, into : *string) : void;

    /**
     * reads a single annotation, this doesn't read '@'
     */
    func readAnnotationIdentifier (&self, into : *string) : void;

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