public struct LocationData {
    var fileId : uint
    var lineStart : uint
    var charStart : uint
    var lineEnd : uint
    var charEnd : uint

    public func start(&self) : Position {
        return Position { line: self.lineStart, character: self.charStart };
    }

    public func end(&self) : Position {
        return Position { line: self.lineEnd, character: self.charEnd };
    }
}
