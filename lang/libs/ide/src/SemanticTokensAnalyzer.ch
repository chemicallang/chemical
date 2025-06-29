@static
public interface SemanticTokensAnalyzer {

    func putAuto(&self, token : *mut Token);

    func getCurrentTokenPtr(&self) : *mut *mut Token;

    func getEndToken(&self) : *mut Token

    func put(
            &self,
            lineNumber : uint32_t,
            lineCharNumber : uint32_t,
            length : uint32_t,
            tokenType : uint32_t,
            tokenModifiers : uint32_t
    )

    func putToken(
            &self,
            token : *mut Token,
            tokenType : uint32_t,
            tokenModifiers : uint32_t
    )

}

public func (analyzer : &mut SemanticTokensAnalyzer) getCurrentToken() : *mut Token {
    return *analyzer.getCurrentTokenPtr()
}

public func (analyzer : &mut SemanticTokensAnalyzer) setCurrentToken(token : *mut Token) {
    *analyzer.getCurrentTokenPtr() = token;
}