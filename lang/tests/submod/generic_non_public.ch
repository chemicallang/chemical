struct GenNonPublicUsedByPublic<T> {
    var value : T
    func give(&self) : T {
        return value + 1;
    }
}

public struct GenPublicUseNonPublic<T> {
    func give(&self) : T {
        var g = GenNonPublicUsedByPublic<T> { value : 83331 }
        return g.give()
    }
}