func traverse_decl(
    decl : *mut CSSDeclaration,
    resolver : *mut SymbolResolver,
    loc : ubigint
) {

    // TODO css value kind should be chemical value to proceed

}

func traverse_multi_decls(
    vec : &std::vector<*mut CSSDeclaration>,
    resolver : *mut SymbolResolver,
    loc : ubigint
) {

    var i = 0
    const total = vec.size()
    while(i < total) {
        const decl = vec.get(i)
        traverse_decl(decl, resolver, loc)
        i++;
    }

}

func traverse_cssom(om : *mut CSSOM, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    // TODO: currently we do not have any chemical items in the om
}