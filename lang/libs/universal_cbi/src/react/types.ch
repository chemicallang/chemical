struct UniversalStateDecl {
    var name : std::string_view
    var initExpr : std::string_view
    var initText : std::string_view
}

struct UniversalTextBinding {
    var stateName : std::string_view
    var path : std::string_view
}

struct UniversalEventBinding {
    var eventName : std::string_view
    var path : std::string_view
    var handlerExpr : std::string_view
}

struct UniversalPropTextBinding {
    var propPath : std::string_view
    var path : std::string_view
}

struct UniversalAttrBinding {
    var attr : *mut MergedAttribute
    var path : std::string_view
}

struct UniversalNestedBinding {
    var componentName : std::string_view
    var path : std::string_view
    var propsExpr : std::string_view
}
