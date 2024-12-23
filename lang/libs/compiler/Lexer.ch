import "./SourceProvider.ch"
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
    var subroutine : (instance : &void, lexer : &Lexer) => Token;
}

@compiler.interface
struct Lexer : LexerState {

    var provider : SourceProvider

    var user_lexer : UserLexerFn;

    var str : SerialStrAllocator

    func getFileAllocator(&self) : *BatchAllocator

    func setUserLexer(&self, instance : *void, subroutine : (instance : &void, lexer : &Lexer) => Token);

    func getEmbeddedToken(&mut self) : Token

}