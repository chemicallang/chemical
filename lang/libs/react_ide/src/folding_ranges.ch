@no_mangle
public func react_foldingRangesPut(analyzer : &FoldingRangeAnalyzer, start : *Token, end : *Token) : *Token {

    var current = start

    var opened_braces = 0;

    while(current != end) {

        if(current.type == JsTokenType.LBrace) {
            opened_braces++;
            analyzer.stackPush(current.position)
        } else if(current.type == JsTokenType.RBrace) {
            if(!analyzer.stackEmpty()) {
                const opening = analyzer.stackPop();
                analyzer.put(opening, current.position, false)
            }
            opened_braces--;
            if(opened_braces == 0) {
                return current;
            }
        }

        current++
    }

    return current;

}