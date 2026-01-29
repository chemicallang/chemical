@compiler.interface
public interface SymbolResolver : ASTDiagnoser {

    func getAnnotationController(&self) : *mut AnnotationController

    func find(&self, view : &std::string_view) : *mut ASTNode;

    func declare(&self, view : &std::string_view, node : *mut ASTNode)

}