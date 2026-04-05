@compiler.interface
public interface SemanticTokensAnalyzer {

    // it returns the next token, from where you should begin (usually token + 1)
    // but if you send a macro token, the macro processor would handle it
    // and would probably return a token after the whole macro has been processed
    func putAuto(&self, token : *mut Token) : *mut Token

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