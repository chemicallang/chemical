@compiler.interface
public interface SymResLinkBody {

    func getSymbolResolver(&self) : *mut SymbolResolver

    func visitNode(&self, node : *mut EmbeddedNode);

    func visitValue(&self, value : *mut EmbeddedValue)

}