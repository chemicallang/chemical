@compiler.interface
public interface SymbolResolver : ASTDiagnoser {

    func getAnnotationController(&self) : *mut AnnotationController

    func find(&self, view : &std::string_view) : *mut ASTNode;

    // this function is very direct, doesn't cause shadowing
    // it will cause an error, even if the current symbol is namespaced
    // and previous symbol is not
    func declare(&self, view : &std::string_view, node : *mut ASTNode)

    // declare the symbol with default shadowing
    // symbol shadowed if this present in namespace
    // by default this should be used
    func declare_tld_default(&self, view : &std::string_view, node : *mut ASTNode)

    // shadows the symbol if already exists, doesn't cause an error
    func declare_or_shadow(&self, view : &std::string_view, node : *mut ASTNode)

    // starts a scope
    func scope_start(&self);
    
    // ends the previously started scope, dropping all symbols introduced between
    func scope_end(&self);

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