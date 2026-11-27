using std::vector;

// Must be rejected: `vector::push` can panic (allocation/bounds).
func bad_alloc() : int [no_panic] {
    var v = vector<int>()
    v.push(1)
    return v.size() as int
}

// Must be rejected: calls the panicking helper above transitively.
func bad_chain() : int [no_panic] {
    return bad_alloc()
}

public func main() : int {
    return bad_chain()
}
