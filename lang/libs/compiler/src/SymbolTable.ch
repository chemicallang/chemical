@compiler.interface
public interface SymbolTable {

    func declare(&self, name : &std::string_view, node : *mut ASTNode);

    func declare_no_shadow(&self, name : &std::string_view, node : *mut ASTNode);

    func scope_start(&self);

    func scope_start_index(&self) : ulong;

    func scope_end(&self);

    func resolve(&self, name : &std::string_view) : *mut ASTNode

}