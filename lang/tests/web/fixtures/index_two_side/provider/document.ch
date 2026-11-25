// The ambient surface this module provides to the universal indexer. A public
// zero-parameter function becomes an ambient root; its return type is retained
// for the Phase 3 member lookup.
public struct Document {
    var title : std::string_view
}

public func document() : Document {
    return Document { title : "provider" }
}
