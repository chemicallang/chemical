/**
 * Runtime implementations for the remaining compiler static interfaces and
 * struct methods that the shared js_parser package references.
 *
 * These are only referenced by symbol-resolution helpers that the runtime
 * `js` package never invokes (they are part of the CBI machinery), so the
 * implementations are stubs that keep the linker satisfied. Inside a compiler
 * plugin (CBI) build the compiler provides the real implementations.
 */
using namespace std;

// ---------------------------------------------------------------------------
// PtrVec (static interface)
// ---------------------------------------------------------------------------

public struct JsRuntimePtrVec {
}

impl PtrVec for JsRuntimePtrVec {
    func _get(&self, i : uint) : *void {
        return null
    }
    func _set(&mut self, i : uint, ptr : *void) {
    }
    func _push(&mut self, ptr : *void) {
    }
    func _erase(&mut self, i : uint) {
    }
    func _data(&self) : **void {
        return null
    }
    func _size(&self) : size_t {
        return 0
    }
}

// ---------------------------------------------------------------------------
// SymResLinkBody (static interface)
// ---------------------------------------------------------------------------

public struct JsRuntimeSymResLinkBody {
}

impl SymResLinkBody for JsRuntimeSymResLinkBody {
    func getSymbolResolver(&self) : *mut SymbolResolver {
        return null
    }
    func getSymbolTable(&self) : *mut SymbolTable {
        return null
    }
    func getAstDiagnoser(&self) : *mut ASTDiagnoser {
        return null
    }
    func visitNode(&self, node : *mut ASTNode) {
    }
    func visitValue(&self, value : *mut Value) {
    }
    func visitEmbeddedNode(&self, node : *mut EmbeddedNode) {
    }
    func visitEmbeddedValue(&self, value : *mut EmbeddedValue) {
    }
}

// ---------------------------------------------------------------------------
// SymbolTable (static interface)
// ---------------------------------------------------------------------------

public struct JsRuntimeSymbolTable {
}

impl SymbolTable for JsRuntimeSymbolTable {
    func declare(&self, name : &std::string_view, node : *mut ASTNode) {
    }
    func declare_no_shadow(&self, name : &std::string_view, node : *mut ASTNode) {
    }
    func scope_start(&self) {
    }
    func scope_start_index(&self) : ulong {
        return 0
    }
    func scope_end(&self) {
    }
    func resolve(&self, name : &std::string_view) : *mut ASTNode {
        return null
    }
}

// ---------------------------------------------------------------------------
// SymbolResolver (static interface, inherits ASTDiagnoser)
// ---------------------------------------------------------------------------

public struct JsRuntimeSymbolResolver {
}

impl SymbolResolver for JsRuntimeSymbolResolver {
    func getAnnotationController(&self) : *mut AnnotationController {
        return null
    }
    func resolve(&self, view : &std::string_view) : *mut ASTNode {
        return null
    }
    func declare(&self, view : &std::string_view, node : *mut ASTNode) {
    }
    func declare_tld_default(&self, view : &std::string_view, node : *mut ASTNode) {
    }
    func declare_or_shadow(&self, view : &std::string_view, node : *mut ASTNode) {
    }
    func scope_start(&self) {
    }
    func scope_end(&self) {
    }
    func getJobBuilder(&self) : ASTBuilder {
        return ASTBuilder { allocator : null, typeBuilder : null }
    }
    func getModBuilder(&self) : ASTBuilder {
        return ASTBuilder { allocator : null, typeBuilder : null }
    }
    func getFileBuilder(&self) : ASTBuilder {
        return ASTBuilder { allocator : null, typeBuilder : null }
    }
    func error(&self, msg : &std::string_view, loc : ubigint) {
    }
}

// ---------------------------------------------------------------------------
// Struct methods referenced by shared helpers (provided as no-mangle symbols)
// ---------------------------------------------------------------------------

@no_mangle
public func compiler_ASTDiagnosererror(diagnoser : *mut ASTDiagnoser, msg : &std::string_view, loc : ubigint) {
}

@no_mangle
public func compiler_ASTNodegetParent(node : *mut ASTNode) : *mut ASTNode {
    return null
}

@no_mangle
public func compiler_ASTNodegetKind(node : *mut ASTNode) : ASTNodeKind {
    return ASTNodeKind.StructDecl
}

@no_mangle
public func compiler_IndexOperatorget_idx_ptr(idx : *mut IndexOperator) : *mut *mut Value {
    return null
}

@no_mangle
public func compiler_ASTBuilderallocate_with_cleanup(builder : *mut ASTBuilder, obj_size : size_t, alignment : size_t, cleanup_fn : (obj : *void) => void) : *mut void {
    const allocator = builder.allocator as *mut ASTAllocator
    return allocator.allocate_with_cleanup(obj_size, alignment, cleanup_fn)
}

// ---------------------------------------------------------------------------
// Lexer / ModuleScope struct methods referenced by the html_parser lexer and
// html_comp helpers. The html lexer only calls unsetUserLexer when leaving a
// chemical embedded block; ModuleScope accessors are used by component-name
// helpers. All are no-ops at runtime.
// ---------------------------------------------------------------------------

@no_mangle
public func compiler_LexerunsetUserLexer(lexer : *mut Lexer) {
}

@no_mangle
public func compiler_ModuleScopegetScopeName(scope : *mut ModuleScope) : std::string_view {
    return std::string_view("")
}

@no_mangle
public func compiler_ModuleScopegetModuleName(scope : *mut ModuleScope) : std::string_view {
    return std::string_view("")
}
