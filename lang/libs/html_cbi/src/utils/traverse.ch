func traverse_element(element : *HtmlElement, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) : bool {
    var i = 0;
    const total = element.children.size()
    while(i < total) {
        const child = element.children.get(i)
        if(child.kind == HtmlChildKind.Element) {
            const elem = child as *HtmlElement
            traverse_element(elem, data, traverse)
        } else if(child.kind == HtmlChildKind.ChemicalValue) {
            const elem = child as *HtmlChemValueChild;
            if(!traverse(data, elem.value)) {
                return false;
            }
        }
        i++;
    }
    return true;
}

func traverse_root(root : *mut HtmlRoot, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    traverse_element(root.element, data, traverse);
}