
public enum AttributeValueKind {
    Text,
    Number,
    Chemical,
    ChemicalValues
}

public struct AttributeValue {

    var kind : AttributeValueKind

}

public struct TextAttributeValue : AttributeValue {

    var text : std::string_view

}

public struct ChemicalAttributeValue : AttributeValue {

    var value : *mut Value

}

public struct ChemicalAttributeValues : AttributeValue {

    var values : std::vector<*mut Value>

}

public struct HtmlAttribute {

    var name : std::string_view

    // when null, it means a boolean attribute
    var value : *AttributeValue

    var loc : ubigint

}