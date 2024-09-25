import "../compiler/Lexer.ch"
import "../compiler/CSTConverter.ch"

func lexMacro(lexer : Lexer*) {
    lexer.lexNumberToken();
}

func parseMacro(converter : CSTConverter*, token : CSTToken*) {
    const contained = token.tokens();
    const interested = contained.get(1);
    converter.make_uint_value(33, token);
}