public struct CSSOM {

    var parent : *mut ASTNode

    var declarations : std::vector<*mut CSSDeclaration>

    var media_queries : std::vector<*mut CSSMediaRule>

    var keyframes : std::vector<*mut CSSKeyframesRule>

    // chemical values
    var dyn_values : std::vector<*mut Value>

    var nested_rules : std::vector<*mut CSSNestedRule>

    var className : std::string_view

    // True when the block's CSS belongs in the shared bundle (preferring the
    // shared assets sink, falling back to the page): `#globalcss` and component
    // `style { }`. Page-level `#css` leaves this false and is always emitted
    // into the page's own CSS.
    var shared : bool = false

    var support : CssSymResSupport

    func is_hashable(&self) : bool {
        return dyn_values.empty() && media_queries.empty() && nested_rules.empty() && keyframes.empty()
    }

}