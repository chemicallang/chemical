import "@std/string_view.ch"
import "@std/vector.ch"
import "@compiler/ASTBuilder.ch"

enum AttributeValueKind {
    Text,
    Number,
    Chemical
}

struct AttributeValue {

    var kind : AttributeValueKind

}

struct TextAttributeValue : AttributeValue {

    var text : std::string_view

}

struct ChemicalAttributeValue : AttributeValue {

    var value : *Value

}

struct HtmlAttribute {

    var name : std::string_view

    var value : *AttributeValue

}

enum HtmlChildKind {
    Text,
    Element,
    ChemicalValue
}

struct HtmlChild {

    var kind : HtmlChildKind

}

struct HtmlChemValueChild : HtmlChild {

    var value : *Value

}

struct HtmlText : HtmlChild {

    var value : std::string_view

}

struct HtmlElement : HtmlChild {

    var name : std::string_view

    var attributes : std::vector<*HtmlAttribute>

    var children : std::vector<*HtmlChild>

    @delete
    func delete(&self) {
        // do nothing
    }

}

struct HtmlRoot {

    var element : *HtmlElement

    var parent : *mut ASTNode

}