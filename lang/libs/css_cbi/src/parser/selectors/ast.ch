
public enum Combinator {
    Descendant, // " "
    Child,      // ">"
    NextSibling,// "+"
    SubsequentSibling, // "~"
    None
}

public enum SimpleSelectorKind {
    Tag,
    Class,
    Id,
    Universal,
    Attribute,
    PseudoClass,
    PseudoElement,
    Ampersand // For nesting
}

public struct SimpleSelector {
    var kind : SimpleSelectorKind
    var value : std::string_view // For tag, class, id names
    // For attributes, we might need more complex data, but string_view for pattern might suffice for now
    // e.g. "[attr=val]"
    var attr : *mut AttributeSelectorData
}

public struct AttributeSelectorData {
    var name : std::string_view
    var operator : std::string_view // =, ~=, |=, etc.
    var value : std::string_view
    var flags : std::string_view // i, s
}

public struct CompoundSelector {
    var simple_selectors : std::vector<*mut SimpleSelector>
}

public struct ComplexSelector {
    var compound : *mut CompoundSelector
    var combinator : Combinator
    var next : *mut ComplexSelector // Recursive list
}

public struct SelectorList {
    var selectors : std::vector<*mut ComplexSelector>
}
