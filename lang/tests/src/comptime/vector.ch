import "/test.ch"

@comptime
func compiler_vector_sum(a : int) : int {
    var vec = compiler::vector<int>();
    vec.push(a);
    vec.push(10);
    vec.push(20);
    vec.push(40);
    vec.remove(3);
    var sum = 0;
    var i = 0;
    while(i < vec.size()) {
        sum += vec.get(i as uint);
        i++;
    }
    return sum
}

@comptime
func compiler_vector_sum_l(a : long) : long {
    var vec = compiler::vector<long>();
    vec.push(a);
    vec.push(10);
    vec.push(20);
    vec.push(40);
    vec.remove(3);
    var sum = 0;
    var i = 0;
    while(i < vec.size()) {
        sum += vec.get(i as uint);
        i++;
    }
    return sum
}

func test_compiler_vector() {
    test("test that compiler vector works", () => {
        return compiler_vector_sum(20) == 50;
    })
    test("test that compiler vector works with longs", () => {
        return compiler_vector_sum_l(20) == 50;
    })
}

