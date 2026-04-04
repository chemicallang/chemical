func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {

        default => {
            analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
        }
    }
}

@no_mangle
public func js_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *Token, end : *Token) : *Token {

    var current = start

    // put and skip the macro token
    analyzer.putToken(current, SemanticTokenTypes.Macro, 0)
    current++

    var opened_braces = 0;

    while(current != end) {

        putToken(analyzer, current)

        if(current.type == TokenType.LBrace) {
            opened_braces++;
        } else if(current.type == TokenType.RBrace) {
            opened_braces--;
            if(opened_braces == 0) {
                return current;
            }
        }

        current++;

    }

    return current;

}