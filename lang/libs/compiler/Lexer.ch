import "./SourceProvider.ch"
import "./LexTokenType.ch"
import "./CSTToken.ch"
import "@compiler/BatchAllocator.ch"
import "@compiler/SerialStrAllocator.ch"

using namespace std;

struct LexerState {

    var other_mode : bool

    var char_mode : bool

    var string_mode : bool

    var comment_mode : bool

    var user_mode : bool

}

struct UserLexerFn {
    var instance : *void
    var subroutine : (instance : *void, lexer : *Lexer) => Token;
}

@compiler.interface
struct Lexer : LexerState {

    var provider : SourceProvider

    var getNextToken : UserLexerFn;

    var str : SerialStrAllocator

    var fileAllocator : &BatchAllocator

}