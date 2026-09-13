struct ASTConverter {

    var builder : *mut ASTBuilder

    // Live codegen diagnoser (Codegen is an ASTDiagnoser). Used to report
    // constructs that cannot be translated, instead of silently dropping them
    // or emitting a runtime error string into the JS bundle.
    var diagnoser : *mut ASTDiagnoser = null

    // Location of the enclosing macro invocation. Used as the diagnostic
    // location when an individual value carries no encoded location.
    var fallback_loc : ubigint = 0

    var support : *mut SymResSupport

    var vec : *mut VecRef<ASTNode>

    var parent : *mut ASTNode

    var str : std::string

    var in_head : bool = false

    // While rendering a parent component's children for SSR, nested component
    // children are emitted as client vnodes by the parent, so they must NOT also
    // emit their own hydration dispatch (that would double-mount).
    var suppress_child_dispatch : bool = false

}