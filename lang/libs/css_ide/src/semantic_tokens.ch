func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
            TokenType.Identifier => {
                analyzer.putToken(token, SemanticTokenTypes.Variable, 0)
            }
            TokenType.ClassName => {
                 analyzer.putToken(token, SemanticTokenTypes.Variable, 0)
            }
            TokenType.Id => {
                 analyzer.putToken(token, SemanticTokenTypes.Variable, 0)
            }
            TokenType.PropertyName => {
                 analyzer.putToken(token, SemanticTokenScopes.SupportTypePropertyNameCss, 0)
            }
            TokenType.Number => {
                analyzer.putToken(token, SemanticTokenTypes.Number, 0)
            }
            TokenType.Comment => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.DoubleQuotedValue => {
                analyzer.putToken(token, SemanticTokenTypes.String, 0)
            }
            TokenType.SingleQuotedValue => {
                analyzer.putToken(token, SemanticTokenTypes.String, 0)
            }
            TokenType.Colon => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.Semicolon => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.Dot => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.Hash => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.At => {
                analyzer.putToken(token, SemanticTokenTypes.Macro, 0)
            }
            TokenType.Important => {
                analyzer.putToken(token, SemanticTokenTypes.Keyword, 0)
            }
            TokenType.LBrace => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.RBrace => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.LParen => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.RParen => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.HexColor => {
                analyzer.putToken(token, SemanticTokenScopes.ConstantOtherColorRgbValue, 0)
            }
            TokenType.Percentage => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.Comma => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.Plus => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.Minus => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.Multiply => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.Divide => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.Equal => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.GreaterThan => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.GreaterThanOrEqual => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.LessThan => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.LessThanOrEqual => {
                analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
            }
            TokenType.ContainsWord => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.ContainsSubstr => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.StartsWith => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.EndsWith => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.DashSeparatedMatch => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            TokenType.GeneralSibling => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
            default => {
                analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
            }
    }
}

@no_mangle
public func css_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *Token, end : *Token) : *Token {

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