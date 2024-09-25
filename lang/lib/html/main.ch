import "../compiler/Lexer.ch"
import "../compiler/CSTConverter.ch"

public func lexMacro(lexer : Lexer*) {
    lexer.lexNumberToken();
}

public func parseMacro(converter : CSTConverter*, token : CSTToken*) {
    const contained = token.tokens();
    const interested = contained.get(1);
    const value = converter.make_uint_value(33, token);
    converter.put_value(value, interested);
}