@compiler.interface
public struct SymbolResolver : ASTDiagnoser {

    func find(&self, view : &std::string_view) : *mut ASTNode;

}