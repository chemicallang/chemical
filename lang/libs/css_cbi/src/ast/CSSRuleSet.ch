enum CSSSelectorKind {
    TagName,
    Class,
    Id
}

struct CSSSelector {

    var kind : CSSSelectorKind

}

struct CSSNamedSelector : CSSSelector {

    var identifier : std::string_view

}

struct CSSRuleSet {

    var selectors : std::vector<*mut CSSSelector>

    var declarations : std::vector<*mut CSSDeclaration>

}