@static
public interface SemanticTokensAnalyzer {

    func putAuto(&self, token : *mut Token);

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