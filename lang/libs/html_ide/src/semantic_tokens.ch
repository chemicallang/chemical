func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
        TokenType.TagName => {
            analyzer.putToken(token, SemanticTokenScopes.EntityNameTag, 0)
        }
        TokenType.AttrName => {
            analyzer.putToken(token, SemanticTokenScopes.EntityOtherAttributeNameHtml, 0)
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
public func html_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *Token, end : *Token) : *Token {

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