func putToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
        JsTokenType.Var, JsTokenType.Const, JsTokenType.Let, JsTokenType.Function, JsTokenType.Return,
        JsTokenType.If, JsTokenType.Else, JsTokenType.For, JsTokenType.While, JsTokenType.Break,
        JsTokenType.Continue, JsTokenType.Switch, JsTokenType.Case, JsTokenType.Default, JsTokenType.Do,
        JsTokenType.Try, JsTokenType.Catch, JsTokenType.Finally, JsTokenType.Throw, JsTokenType.Typeof,
        JsTokenType.Void, JsTokenType.Delete, JsTokenType.In, JsTokenType.InstanceOf, JsTokenType.New,
        JsTokenType.Of, JsTokenType.Async, JsTokenType.Await, JsTokenType.Class, JsTokenType.Extends,
        JsTokenType.Super, JsTokenType.Static, JsTokenType.Import, JsTokenType.Export, JsTokenType.Yield,
        JsTokenType.Debugger => {
            analyzer.putToken(token, SemanticTokenTypes.Keyword, 0)
        }
        JsTokenType.True, JsTokenType.False, JsTokenType.Null, JsTokenType.Undefined => {
            analyzer.putToken(token, SemanticTokenTypes.Keyword, 0)
        }
        JsTokenType.Identifier => {
            analyzer.putToken(token, SemanticTokenTypes.Variable, 0)
        }
        JsTokenType.Number => {
            analyzer.putToken(token, SemanticTokenTypes.Number, 0)
        }
        JsTokenType.String, JsTokenType.TemplateLiteral => {
            analyzer.putToken(token, SemanticTokenTypes.String, 0)
        }
        JsTokenType.Equal, JsTokenType.Plus, JsTokenType.Minus, JsTokenType.Star, JsTokenType.Slash,
        JsTokenType.EqualEqual, JsTokenType.NotEqual, JsTokenType.LessThan, JsTokenType.GreaterThan,
        JsTokenType.LessThanEqual, JsTokenType.GreaterThanEqual, JsTokenType.Exclamation, JsTokenType.Arrow,
        JsTokenType.LogicalAnd, JsTokenType.LogicalOr, JsTokenType.Question, JsTokenType.PlusPlus,
        JsTokenType.MinusMinus, JsTokenType.PlusEqual, JsTokenType.MinusEqual, JsTokenType.StarEqual,
        JsTokenType.SlashEqual, JsTokenType.BitwiseNot, JsTokenType.BitwiseAnd, JsTokenType.BitwiseOr,
        JsTokenType.BitwiseXor, JsTokenType.LeftShift, JsTokenType.RightShift, JsTokenType.RightShiftUnsigned,
        JsTokenType.ThreeDots => {
            analyzer.putToken(token, SemanticTokenTypes.Operator, 0)
        }
        JsTokenType.ChemicalStart => {
            analyzer.putToken(token, SemanticTokenTypes.Macro, 0)
        }
        JsTokenType.JSXText => {
            analyzer.putToken(token, SemanticTokenScopes.TextHtmlDerivative, 0)
        }
        default => {
            // Not highlighted
        }
    }
}

@no_mangle
public func react_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *mut Token, end : *Token) : *Token {

    var current = start

    // put and skip the macro token
    analyzer.putToken(current, SemanticTokenTypes.Macro, 0)
    current++

    var opened_braces = 0;

    while(current != end) {

        putToken(analyzer, current)

        if(current.type == JsTokenType.LBrace) {
            opened_braces++;
        } else if(current.type == JsTokenType.RBrace) {
            opened_braces--;
            if(opened_braces == 0) {
                return current;
            }
        }

        current++;

    }

    return current;

}