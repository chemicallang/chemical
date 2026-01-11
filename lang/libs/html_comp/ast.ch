public struct ComponentParam {
    var name : std::string_view
    var is_optional : bool
}

public struct ComponentSignature {
    var name : std::string_view
    var params : std::vector<ComponentParam>
}