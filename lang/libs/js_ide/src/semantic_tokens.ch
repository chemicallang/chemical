func putJsToken(analyzer : &mut SemanticTokensAnalyzer, token : *mut Token) {
    switch(token.type) {
        JsTokenType.Var, JsTokenType.Const, JsTokenType.Let, JsTokenType.Function, JsTokenType.Class => {
            analyzer.putToken(token, SemanticTokenScopes.StorageType, 0)
        }
        JsTokenType.If, JsTokenType.Else, JsTokenType.Return, JsTokenType.For, JsTokenType.While,
        JsTokenType.Break, JsTokenType.Continue, JsTokenType.Switch, JsTokenType.Case, JsTokenType.Default,
        JsTokenType.Do, JsTokenType.Try, JsTokenType.Catch, JsTokenType.Finally, JsTokenType.Throw => {
            analyzer.putToken(token, SemanticTokenScopes.KeywordControl, 0)
        }
        JsTokenType.Async, JsTokenType.Await, JsTokenType.Export, JsTokenType.Import, JsTokenType.Yield,
        JsTokenType.Debugger, JsTokenType.Static, JsTokenType.Extends, JsTokenType.New, JsTokenType.Of,
        JsTokenType.Typeof, JsTokenType.Void, JsTokenType.Delete, JsTokenType.In, JsTokenType.InstanceOf => {
            analyzer.putToken(token, SemanticTokenScopes.Keyword, 0)
        }
        JsTokenType.This => {
            analyzer.putToken(token, SemanticTokenScopes.VariableLanguageThis, 0)
        }
        JsTokenType.Super => {
            analyzer.putToken(token, SemanticTokenScopes.VariableLanguage, 0)
        }
        JsTokenType.True, JsTokenType.False, JsTokenType.Null, JsTokenType.Undefined => {
            analyzer.putToken(token, SemanticTokenScopes.ConstantLanguage, 0)
        }
        JsTokenType.Identifier => {
            analyzer.putToken(token, SemanticTokenTypes.Variable, 0)
        }
        JsTokenType.Number => {
            analyzer.putToken(token, SemanticTokenScopes.ConstantNumeric, 0)
        }
        JsTokenType.String, JsTokenType.TemplateLiteral => {
            analyzer.putToken(token, SemanticTokenScopes.String, 0)
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
        default => {
            // Unhighlighted
        }
    }
}

@no_mangle
public func js_semanticTokensPut(analyzer : &mut SemanticTokensAnalyzer, start : *mut Token, end : *Token) : *Token {
    var current = start
    analyzer.putToken(current, SemanticTokenTypes.Macro, 0)
    current++

    var opened_braces = 0

    while(current != end) {
        if(current.type == JsTokenType.LBrace as int) {
            opened_braces++
        } else if(current.type == JsTokenType.RBrace as int) {
            opened_braces--
            if(opened_braces == 0) {
                putJsToken(analyzer, current)
                current++
                return current
            }
        }
        putJsToken(analyzer, current)
        current++
    }
    return current
}