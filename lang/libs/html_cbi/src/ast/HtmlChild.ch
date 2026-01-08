enum HtmlChildKind {
    Text,
    Element,
    Comment,
    ChemicalNode,
    ChemicalValue,
    IfStatement
}

struct HtmlChild {

    var kind : HtmlChildKind

}

struct HtmlChemNodeChild : HtmlChild {

    var node : *mut ASTNode

}