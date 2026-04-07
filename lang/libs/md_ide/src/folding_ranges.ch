@no_mangle
public func md_foldingRangesPut(analyzer : &mut FoldingRangeAnalyzer, start : *mut Token, end : *Token) : *Token {

    var current = start

    var opened_braces = 0;

    while(current != end) {

        if(current.type == MdTokenType.FencedCodeStart) {
            analyzer.stackPush(current.position)
        } else if(current.type == MdTokenType.FencedCodeEnd) {
            if(!analyzer.stackEmpty()) {
                const opening = analyzer.stackPop();
                analyzer.put(opening, current.position, false)
            }
        } else if(current.type == MdTokenType.LBrace) {
            opened_braces++;
            analyzer.stackPush(current.position)
        } else if(current.type == MdTokenType.RBrace) {
            if(!analyzer.stackEmpty()) {
                const opening = analyzer.stackPop();
                analyzer.put(opening, current.position, false)
            }
            opened_braces--;
            if(opened_braces == 0) {
                current++;
                return current;
            }
        }

        current++
    }

    return current;

}