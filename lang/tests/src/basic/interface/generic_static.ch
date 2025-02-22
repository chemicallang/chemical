@static
interface GenStatSummer<T> {

    func sum(&self) : T

}

func (summer : &mut GenStatSummer<short>) gen_stat_short_mul_sum() : short {
    return summer.sum() * 2;
}

func (summer : &mut GenStatSummer<int>) gen_stat_int_mul_sum() : int {
    return summer.sum() * 3;
}

struct ImplShortGenStatSummer : GenStatSummer<short> {

    var a : int
    var b : int

    @override
    func sum(&self) : short {
        return a + b + 2;
    }

}

struct ImplIntGenStatSummer : GenStatSummer<int> {

    var a : int
    var b : int

    @override
    func sum(&self) : int {
        return a + b + 3;
    }

}

func test_generic_static_interfaces() {

    test("generic static interfaces work - 1", () => {
        var s = ImplShortGenStatSummer { a : 4, b : 8 }
        return s.sum() == 14
    })

    test("generic static interfaces work - 2", () => {
        var s = ImplIntGenStatSummer { a : 10, b : 30 }
        return s.sum() == 43
    })

    test("generic static interfaces work through extension methods - 1", () => {
        var s = ImplShortGenStatSummer { a : 4, b : 4 }
        return s.gen_stat_short_mul_sum() == 20
    })

    test("generic static interfaces work through extension methods - 2", () => {
        var s = ImplIntGenStatSummer { a : 5, b : 8 }
        return s.gen_stat_int_mul_sum() == 48
    })

}