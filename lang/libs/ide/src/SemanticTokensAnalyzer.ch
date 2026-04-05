@compiler.interface
public interface SemanticTokensAnalyzer {

    func putAuto(&self, token : *mut Token);

    func put(
            &self,
            lineNumber : u32,
            lineCharNumber : u32,
            length : u32,
            tokenType : u32,
            tokenModifiers : u32
    )

    func putToken(
            &self,
            token : *mut Token,
            tokenType : u32,
            tokenModifiers : u32
    )

}