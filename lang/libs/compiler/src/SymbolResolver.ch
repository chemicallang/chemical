@compiler.interface
public struct SymbolResolver : ASTDiagnoser {

    func find(&self, view : &std::string_view) : *mut ASTNode;

    func declare(&self, view : &std::string_view, node : *mut ASTNode)

}