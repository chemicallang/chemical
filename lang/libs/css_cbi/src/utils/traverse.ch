func traverse_multi_values(
    vec : &std::vector<*mut Value>,
    data : *void,
    traverse : (data : *void, item : *mut ASTAny) => bool
) {
    var start = vec.data()
    const end = start + vec.size()
    while(start != end) {
        traverse(data, *start)
        start++
    }
}

func traverse_cssom(om : *mut CSSOM, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    traverse_multi_values(om.dyn_values, data, traverse)
}