import "../test.ch"

@comptime
func compiler_vector_sum(a : int) {
    var vec = compiler::Vector();
    vec.push(a);
    vec.push(10);
    vec.push(20);
    vec.push(40);
    vec.erase(3);
    var sum = 0;
    var i = 0;
    while(i < vec.size()) {
        sum += vec.get(i);
        i++;
    }
    return sum
}

func test_compiler_vector() {
    test("test that compiler vector works", () => {
        return compiler_vector_sum(20) == 50;
    })
}

