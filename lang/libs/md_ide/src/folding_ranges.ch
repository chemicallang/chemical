@no_mangle
public func md_foldingRangesPut(analyzer : &mut FoldingRangeAnalyzer, start : *mut Token, end : *Token) : *Token {

    var current = start

    while(current != end) {

        if(current.type == MdTokenType.FencedCodeStart) {
            analyzer.stackPush(current.position)
        } else if(current.type == MdTokenType.FencedCodeEnd) {
            if(!analyzer.stackEmpty()) {
                const opening = analyzer.stackPop();
                analyzer.put(opening, current.position, false)
            }
        } else if(current.type == MdTokenType.EndMd) {
            current++;
            return current;
        }

        current++
    }

    return current;

}