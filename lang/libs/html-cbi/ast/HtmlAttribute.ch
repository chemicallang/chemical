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