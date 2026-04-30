
struct CSSKeyframe {
    var selector : std::string_view // "from", "to", or "50%"
    var declarations : std::vector<*mut CSSDeclaration>
}

struct CSSKeyframesRule {
    var name : std::string_view
    var keyframes : std::vector<*mut CSSKeyframe>
    var parent : *mut ASTNode
}
