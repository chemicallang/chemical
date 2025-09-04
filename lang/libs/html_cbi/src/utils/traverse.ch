func traverse_child(child : *mut HtmlChild, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) : bool {
    if(child.kind == HtmlChildKind.Element) {
        const elem = child as *HtmlElement
        return traverse_element(elem, data, traverse)
    } else if(child.kind == HtmlChildKind.ChemicalValue) {
        const elem = child as *HtmlChemValueChild;
        return traverse(data, elem.value)
    }
}

func traverse_element(element : *HtmlElement, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) : bool {

    var i = 0;
    const attrsTotal = element.attributes.size()
    while(i < attrsTotal) {
        const attr = element.attributes.get(i as size_t)
        if(attr.value != null && attr.value.kind == AttributeValueKind.Chemical) {
            const value = attr.value as *mut ChemicalAttributeValue
            if(!traverse(data, value.value)) {
                return false;
            }
        }
        i++;
    }

    i = 0;
    const total = element.children.size()
    while(i < total) {
        const child = element.children.get(i as size_t)
        if(!traverse_child(child, data, traverse)) {
            return false;
        }
        i++;
    }
    return true;
}

func traverse_root(root : *mut HtmlRoot, data : *void, traverse : (data : *void, item : *mut ASTAny) => bool) {
    if(root.element != null) {
        traverse_child(root.element, data, traverse);
    }
}