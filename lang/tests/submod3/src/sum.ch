public func submod3_sum(a : int, b : int) : int {
    return a + b + 11;
}

public func submod3_delgated_sum(a : int, b : int) : int {
    return submod_mod_sum(a, b)
}

public func example_sum_caller() : int {
    return example_sum(10, 10)
}