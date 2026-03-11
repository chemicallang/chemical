@compiler.interface
public interface SymResLinkSignature {

    func getSymbolResolver(&self) : *mut SymbolResolver

    func visitNode(&self, node : *mut ASTNode);

    func visitValue(&self, value : *mut Value)

    func visitEmbeddedNode(&self, node : *mut EmbeddedNode);

    func visitEmbeddedValue(&self, value : *mut EmbeddedValue)

}