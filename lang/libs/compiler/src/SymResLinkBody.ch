@compiler.interface
public interface SymResLinkBody {

    func getSymbolResolver(&self) : *mut SymbolResolver

    func getSymbolTable(&self) : *mut SymbolTable

    func getAstDiagnoser(&self) : *mut ASTDiagnoser

    func visitNode(&self, node : *mut ASTNode);

    func visitValue(&self, value : *mut Value)

    func visitEmbeddedNode(&self, node : *mut EmbeddedNode);

    func visitEmbeddedValue(&self, value : *mut EmbeddedValue)

}