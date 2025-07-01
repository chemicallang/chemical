func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
        TokenType.Identifier => {
            analyzer.putToken(token, SemanticTokenTypes.Keyword, 0)
        }
        TokenType.Text => {
            analyzer.putToken(token, SemanticTokenScopes.TextHtmlDerivative, 0)
        }
        TokenType.Number => {
            analyzer.putToken(token, SemanticTokenTypes.Number, 0)
        }
        TokenType.LessThan => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionTagBeginHtml, 0)
        }
        TokenType.GreaterThan => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionTagEndHtml, 0)
        }
        TokenType.LBrace => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        TokenType.RBrace => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        TokenType.Equal => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationSeparatorKeyValue, 0)
        }
        TokenType.SingleQuotedValue => {
            analyzer.putToken(token, SemanticTokenScopes.StringQuotedSingleHtml, 0)
        }
        TokenType.DoubleQuotedValue => {
            analyzer.putToken(token, SemanticTokenScopes.StringQuotedDoubleHtml, 0)
        }
        TokenType.FwdSlash => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionTagEndHtml, 0)
        }
        TokenType.DeclarationStart => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        TokenType.CommentStart => {
            analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
        }
        TokenType.CommentText => {
            analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
        }
        default => {
            analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
        }
    }
}

@no_mangle
public func html_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer) {

    var currPtr = analyzer.getCurrentTokenPtr();
    const endToken = analyzer.getEndToken();
    var curr = *currPtr

    // put and skip the macro token
    analyzer.putToken(curr, SemanticTokenTypes.Macro, 0)
    curr = curr + 1;

    var opened_braces = 0;

    while(curr != endToken) {

        putToken(analyzer, curr)

        if(curr.type == TokenType.LBrace) {
            opened_braces++;
        } else if(curr.type == TokenType.RBrace) {
            opened_braces--;
            if(opened_braces == 0) {
                return;
            }
        }

        curr = curr + 1;
        *currPtr = curr;
    }

}