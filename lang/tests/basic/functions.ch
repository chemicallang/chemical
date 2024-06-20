import "../test.ch"

struct TestStruct1 {
    var x : int
    var y : int
}

func test_struct() : TestStruct1 {
    return TestStruct1 {
        x : 100,
        y : 100
    }
}

struct MemLamb {

    var lamb : () => TestStruct1

    func mem_func_test() : TestStruct1 {
        return TestStruct1 {
            x : 200,
            y : 200
        }
    }

}

func (mem : MemLamb*) ext_func_test() : TestStruct1 {
    return TestStruct1 {
        x : 300,
        y : 300
    }
}

func test_functions() {
    test("test struct member access in chain, tld function return", () => {
        return test_struct().x == 100;
    })
    test("test struct member access in chain, lambda function return", () => {
        var lamb : () => TestStruct1 = test_struct;
        return lamb().x == 100;
    })
    test("test struct member access in chain, member lambda return", () => {
        var mem = MemLamb {
            lamb : test_struct
        }
        return mem.lamb().x == 100;
    })
    test("test struct member access in chain, member function return", () => {
        var mem = MemLamb {}
        return mem.mem_func_test().x == 200;
    })
    test("test struct member access in chain, extension function return", () => {
        var mem = MemLamb {}
        return mem.ext_func_test().x == 300;
    })
}