protected struct GenProtectedUsedByPublic<T> {
    var value : T
    func give(&self) : T {
        return value + 1;
    }
}

protected func <T> protected_sum_it_(a : T, b : T) : T {
    return a + b;
}

public struct GenPublicUseNonPublic<T> {
    func give(&self) : T {
        var g = GenProtectedUsedByPublic<T> { value : 83331 }
        var c = protected_sum_it_<T>(1, 1)
        return g.give() + c
    }
}