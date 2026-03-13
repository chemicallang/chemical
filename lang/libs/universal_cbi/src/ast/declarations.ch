public struct JsVarDecl {
    var base : JsNode
    var name : std::string_view // Simple name
    var pattern : *mut JsNode    // Destructuring pattern (ArrayDestructuring)
    var value : *mut JsNode
    var keyword : std::string_view
}

public struct JsFunctionDecl {
    var base : JsNode
    var name : std::string_view
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var is_async : bool
    var is_generator : bool
}

public struct JsClassMethod {
    var name : std::string_view
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var is_static : bool
}

public struct JsClassDecl {
    var base : JsNode
    var name : std::string_view
    var superClass : std::string_view // optional
    var methods : std::vector<JsClassMethod>
}

public struct JsImport {
    var base : JsNode
    var source : std::string_view
    var specifiers : std::vector<ImportSpecifier>
}

public struct ImportSpecifier {
    var imported : std::string_view
    var local : std::string_view
}

public struct JsExport {
    var base : JsNode
    var declaration : *mut JsNode
    var is_default : bool
}
