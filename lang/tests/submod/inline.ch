@inline
public func ext_inline_sum_1(a : int, b : int) : int {
    return a + b + 1;
}

@inline.always
public func ext_inline_sum_2(a : int, b : int) : int {
    return a + b + 1;
}

@noinline
public func ext_inline_sum_3(a : int, b : int) : int {
    return a + b + 1;
}