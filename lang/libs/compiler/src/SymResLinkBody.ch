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

public func (visitor : &mut SymResLinkBody) resolve(name : &std::string_view) : *mut ASTNode {
    const res = visitor.getSymbolResolver();
    const f = res.resolve(name)
    if(f != null) return f;
    const t = visitor.getSymbolTable()
    return t.resolve(name)
}

public func (visitor : &mut SymResLinkBody) error(msg : &std::string_view, loc : ubigint) {
    visitor.getAstDiagnoser().error(msg, loc);
}