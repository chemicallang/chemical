public struct Document {
    var title : std::string_view
}

public func document() : Document {
    return Document { title : "provider" }
}
