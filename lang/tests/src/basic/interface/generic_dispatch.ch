interface InterfaceGenericDispatchTestable {

    func sum_it(&self) : int

}

struct InterfaceGenericDispatchTestOneImpl : InterfaceGenericDispatchTestable {

    var i : int

    @override
    func sum_it(&self) : int {
        return 107 + i;
    }

}

struct InterfaceGenericDispatchTestTwoImpl : InterfaceGenericDispatchTestable {

    var i : int

    @override
    func sum_it(&self) : int {
        return 875 + i;
    }

}

func <T : InterfaceGenericDispatchTestable> call_sum_it(value : &T) : int {
    return value.sum_it();
}

func test_interface_generic_dispatch() {
    test("generic dispatch using generics works - 1", () => {
        var x = InterfaceGenericDispatchTestOneImpl { i : 3 }
        return call_sum_it(x) == 110
    })
    test("generic dispatch using generics works - 2", () => {
        var x = InterfaceGenericDispatchTestTwoImpl { i : 5 }
        return call_sum_it(x) == 880
    })
}