
func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
        MdTokenType.Hash => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionHeadingMarkdown, 0)
        }
        MdTokenType.Text => {
            analyzer.putToken(token, SemanticTokenScopes.None, 0)
        }
        MdTokenType.Star => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionItalic, 0)
        }
        MdTokenType.Underscore => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionItalic, 0)
        }
        MdTokenType.LBracket => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionMetadataMarkdown, 0)
        }
        MdTokenType.RBracket => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionMetadataMarkdown, 0)
        }
        MdTokenType.LParen => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionMetadataMarkdown, 0)
        }
        MdTokenType.RParen => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionMetadataMarkdown, 0)
        }
        MdTokenType.Backtick => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionRawMarkdown, 0)
        }
        MdTokenType.GreaterThan => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionQuoteBeginMarkdown, 0)
        }
        MdTokenType.FencedCodeStart => {
            analyzer.putToken(token, SemanticTokenScopes.MarkupInlineRawMarkdown, 0)
        }
        MdTokenType.FencedCodeEnd => {
            analyzer.putToken(token, SemanticTokenScopes.MarkupInlineRawMarkdown, 0)
        }
        MdTokenType.CodeContent => {
            analyzer.putToken(token, SemanticTokenScopes.MarkupInlineRawMarkdown, 0)
        }
        MdTokenType.Number => {
            analyzer.putToken(token, SemanticTokenScopes.MarkupListNumberedMarkdown, 0)
        }
        MdTokenType.Dash => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionListMarkdown, 0)
        }
        MdTokenType.Plus => {
            analyzer.putToken(token, SemanticTokenScopes.PunctuationDefinitionListMarkdown, 0)
        }
        MdTokenType.LBrace => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        MdTokenType.RBrace => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        MdTokenType.EndMd => {
            analyzer.putToken(token, SemanticTokenTypes.Macro, 0)
        }
        default => {
            analyzer.putToken(token, SemanticTokenTypes.Comment, 0)
        }
    }
}

@no_mangle
public func md_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *mut Token, end : *Token) : *Token {

    var current = start

    // put and skip the macro token (#md)
    analyzer.putToken(current, SemanticTokenTypes.Macro, 0)
    current++

    while(current != end) {

        putToken(analyzer, current)

        if(current.type == MdTokenType.EndMd) {
            current++;
            return current;
        }

        current++;

    }

    return current;

}