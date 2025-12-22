enum HtmlChildKind {
    Text,
    Element,
    Comment,
    ChemicalValue,
    IfStatement
}

struct HtmlChild {

    var kind : HtmlChildKind

}