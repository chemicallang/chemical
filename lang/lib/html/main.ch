import "../Lexer.ch"
import "../CSTConverter.ch"

func lexMacro(lexer : Lexer*) {
    lexer.lexNumberToken();
}

func parseMacro(converter : CSTConverter*, token : CSTToken*) {
    const contained = token.tokens();
    const interested = container.get(1);
    converter.make_uint_value(33, token);
}