@compiler.interface
public struct SymbolResolver : ASTDiagnoser {

    func getAnnotationController(&self);

    func find(&self, view : &std::string_view) : *mut ASTNode;

    func declare(&self, view : &std::string_view, node : *mut ASTNode)

}