using namespace std;

public struct LexerState {

    var other_mode : bool

    var user_mode : bool

}

public type UserLexerSubroutineType = (instance : &void, lexer : &Lexer) => Token

public struct UserLexerFn {
    var instance : *void
    var subroutine : UserLexerSubroutineType;
}

@compiler.interface
public struct Lexer : LexerState {

    var provider : SourceProvider

    var user_lexer : UserLexerFn;

    var str : SerialStrAllocator

    func getFileAllocator(&self) : *BatchAllocator

    func setUserLexer(&self, instance : *void, subroutine : UserLexerSubroutineType);

    func unsetUserLexer(&self);

    func getEmbeddedToken(&mut self) : Token

}