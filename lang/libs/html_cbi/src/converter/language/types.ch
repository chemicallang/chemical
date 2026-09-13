struct ASTConverter {

    var builder : *mut ASTBuilder

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