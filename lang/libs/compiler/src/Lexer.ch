import "./SourceProvider.ch"
import "@compiler/BatchAllocator.ch"
import "@compiler/SerialStrAllocator.ch"
import "@compiler/Token.ch"

using namespace std;

public struct LexerState {

    var other_mode : bool

    var user_mode : bool

}

public struct UserLexerFn {
    var instance : *void
    var subroutine : (instance : &void, lexer : &Lexer) => Token;
}

@compiler.interface
public struct Lexer : LexerState {

    var provider : SourceProvider

    var user_lexer : UserLexerFn;

    var str : SerialStrAllocator

    func getFileAllocator(&self) : *BatchAllocator

    func setUserLexer(&self, instance : *void, subroutine : (instance : &void, lexer : &Lexer) => Token);

    func unsetUserLexer(&self);

    func getEmbeddedToken(&mut self) : Token

}