import "@std/vector.ch"

public func give_submod2_rand_num() : int {
    return 83838;
}

struct MyInternalStructBeingUsed {
    var x : int
}

// what happens is, this type maybe declared in tests module
// however since this type is internal, it will be disposed (in previous design)
// (in new design, we don't dispose internal / private struct types)
public func use_internal_struct_with_public_imported_generic() {
    var vec = std::vector<MyInternalStructBeingUsed>()
}