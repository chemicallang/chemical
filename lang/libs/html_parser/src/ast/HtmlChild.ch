public enum HtmlChildKind {
    Text,
    Element,
    Comment,
    ChemicalNode,
    ChemicalValue,
    IfStatement
}

public struct HtmlChild {

    var kind : HtmlChildKind

}

public struct HtmlChemNodeChild : HtmlChild {

    var node : *mut ASTNode

}