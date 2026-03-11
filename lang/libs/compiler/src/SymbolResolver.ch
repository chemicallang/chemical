@compiler.interface
public interface SymbolResolver : ASTDiagnoser {

    func getAnnotationController(&self) : *mut AnnotationController

    func find(&self, view : &std::string_view) : *mut ASTNode;

    func declare(&self, view : &std::string_view, node : *mut ASTNode)

    // shadows the symbol if already exists, doesn't cause an error
    func declare_or_shadow(&self, view : &std::string_view, node : *mut ASTNode)

    // starts a scope
    func scope_start(&self);
    
    // ends the previously started scope, dropping all symbols introduced between
    func scope_end(&self);

    // symbol still won't retain in dependent modules
    // to retain use make_top_level_embedded_node, which provides cross
    // module declarer
    func declare_exported(&self, view : &std::string_view, node : *mut ASTNode)

    // use this, if you want allocations to persist across modules
    func getJobBuilder(&self) : ASTBuilder

    // use this, if you don't want allocations to persist across modules
    // this will improve speed for non-exported nodes
    func getModBuilder(&self) : ASTBuilder

    // the fastest allocator, probably allocations will persist only in files
    // escaping symbols will die as soon as code generation completes on files
    // use carefully
    func getFileBuilder(&self) : ASTBuilder

}