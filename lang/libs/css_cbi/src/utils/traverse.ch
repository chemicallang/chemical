func traverse_decl(
    decl : *mut CSSDeclaration,
    data : *void,
    traverse : (data : *void, item : *mut ASTAny) => bool
) {
    if(decl.value.kind == CSSValueKind.ChemicalValue) {
        traverse(data, decl.value.data as *mut ASTAny);
    }
}

func traverse_multi_decls(
    vec : &std::vector<*mut CSSDeclaration>,
    data : *void,
    traverse : (data : *void, item : *mut ASTAny) => bool
) {
    var start = vec.data()
    const end = start + vec.size()
    while(start != end) {
        const decl = *start;
        traverse_decl(decl, data, traverse)
        start++
    }
}

func traverse_cssom(om : *mut CSSOM, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    traverse_multi_decls(om.declarations, data, traverse)
}