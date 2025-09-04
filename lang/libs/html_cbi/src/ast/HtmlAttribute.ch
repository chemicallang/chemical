
enum AttributeValueKind {
    Text,
    Number,
    Chemical,
    ChemicalValues
}

struct AttributeValue {

    var kind : AttributeValueKind

}

struct TextAttributeValue : AttributeValue {

    var text : std::string_view

}

struct ChemicalAttributeValue : AttributeValue {

    var value : *mut Value

}

struct ChemicalAttributeValues : AttributeValue {

    var values : std::vector<*mut Value>

}

struct HtmlAttribute {

    var name : std::string_view

    // when null, it means a boolean attribute
    var value : *AttributeValue

}