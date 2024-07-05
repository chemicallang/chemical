import "./Operation.ch"
import "../../std/string.ch"

@cbi:global("compiler")
typealias String = char*;

/**
 * provides access to the source code provided by the user
 */
@cbi:global("compiler")
struct SourceProvider {

    /**
     * gets the current pos of the stream
     */
    var currentPosition : (&self) => uint;

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    var readCharacter : (&self) => char;

    /**
     * checks the stream is at the end
     * please also use both peek() == -1
     */
    var eof : (&self) => bool;

    /**
     * peaks the character to read
     */
    var peek : (&self) => char

    /**
     * peaks the character at current pos + ahead
     */
    var peek_at : (&self, offset : int) => char;

    /**
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    var readUntil : (&self, stop : char) => String;

    /**
     * if text is present at current pos in the stream, increments the stream with text.length()
     * @param text to increment
     * @param peek peeks only, doesn't increment
     * @return true if incremented by text length otherwise false
     */
    var increment : (&self, text : String, peek : bool) => bool;

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    var increment_char : (&self, c : char) => bool;

    /**
     * this will read all the text from current position to end in a string and return it
     * useful for debugging only
     */
    var readAllFromHere : (&self) => String

    /**
     * get zero-based current line number
     */
    var getLineNumber : (&self) => uint;

    /**
     * get zero-based character number
     */
    var getLineCharNumber : (&self) => uint

    /**
     * will read everything to the given string
     *
     * will not stop if the stream doesn't end or there's a backslash before stopAt character
     * useful when reading a string token which must not stop at \"
     *
     * will also append the last stopAt character into value
     */
    var readEscaping : (&self, value : String*, stopAt : char) => void;

    /**
     * reads all characters into a string until char occurs
     * @return the string that was found
     */
    var readAnything : (&self, until : char) => String;

    /**
     * reads a alphabetical string
     */
    var readAlpha : (&self) => String;

    /**
     * reads an unsigned integer as string, returns "" if no integer found
     */
    var readUnsignedInt : (&self) => String;

    /**
     * reads a number from the stream
     */
    var readNumber : (&self) => string;

    /**
     * reads a alphanumeric string
     */
    var readAlphaNum : (&self) => String;

    /**
     * reads a single identifier
     */
    var readIdentifier : (&self) => String;

    /**
     * reads a single annotation into given string, this doesn't read '@'
     */
    var readAnnotationIdentifierInto : (&self, into : String*) => void;

    /**
     * reads a single annotation, this doesn't read '@'
     */
    var readAnnotationIdentifier : (&self) => String;

    /**
     * reads whitespaces, returns how many whitespaces were read
     */
    var readWhitespaces : (&self) => uint;

    /**
     * @return whether there's a newline at current position
     */
    var hasNewLine : (&self) => bool;

    /**
     * @return whether new line characters were read
     */
    var readNewLineChars : (&self) => bool;

    /**
     * reads all whitespaces along with new lines
     */
    var readWhitespacesAndNewLines : (&self) => void;

}