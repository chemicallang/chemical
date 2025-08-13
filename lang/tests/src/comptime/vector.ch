comptime func compiler_vector_sum(a : int) : int {
    var vec = intrinsics::vector<int>();
    vec.push(a);
    vec.push(10);
    vec.push(20);
    vec.push(40);
    vec.remove(3);
    var sum = 0;
    var i = 0;
    while(i < vec.size()) {
        sum += vec.get(i as uint) as int;
        i++;
    }
    return sum
}

comptime func compiler_vector_sum_l(a : long) : long {
    var vec = intrinsics::vector<long>();
    vec.push(a);
    vec.push(10);
    vec.push(20);
    vec.push(40);
    vec.remove(3);
    var sum = 0;
    var i = 0;
    while(i < vec.size()) {
        sum += vec.get(i as uint) as long;
        i++;
    }
    return sum
}

func test_compiler_vector() {
    test("compiler vector works", () => {
        return compiler_vector_sum(20) == 50;
    })
    test("compiler vector works with longs", () => {
        return compiler_vector_sum_l(20) == 50;
    })
}

