struct func_name_hiding1_inherited {
    func give(&self) : int {
        return 758
    }
}

struct hider_func_name_hiding : func_name_hiding1_inherited {
    func give_proxy1(&self) : int {
        return give()
    }
    func give(&self) : int {
        return 743
    }
    func give_proxy2(&self) : int {
        return give()
    }
}

interface name_provider_interface_give_hide_test {
    func give(&self) : int
}

struct func_name_provider_struct_concrete {
    func give(&self) : int {
        return 100
    }
}

struct test_for_impl_func_name_hiding {
    func give(&self) : int {
        return 34
    }
}

impl name_provider_interface_give_hide_test for test_for_impl_func_name_hiding {
    func give(&self) : int {
        return 300;
    }
}

struct interface_extension_adopted_method_test {
    func give(&self) : int {
        return 8384
    }
}

interface interface_with_extension_method_trynna_hide {

}

func <T : interface_with_extension_method_trynna_hide> (i : &T) give() : int {
    return 829
}

impl interface_with_extension_method_trynna_hide for interface_extension_adopted_method_test {}

func test_name_hiding() {
    test("function name hiding works properly - 1", () => {
        var s = hider_func_name_hiding {}
        return s.give() == 743
    })
    test("function name hiding works properly - 2", () => {
        var s = hider_func_name_hiding {}
        return s.give_proxy1() == 743
    })
    test("function name hiding works properly - 3", () => {
        var s = hider_func_name_hiding {}
        return s.give_proxy2() == 743
    })
    test("impl of interface doesn't hide existing function name", () => {
        var f = test_for_impl_func_name_hiding {}
        return f.give() == 34;
    })
    test("impl of interface doesn't hide existing function name", () => {
        var f = test_for_impl_func_name_hiding {}
        return f.give() == 34;
    })
    test("impl of interface with extension method doesn't hide existing function name", () => {
        var f = interface_extension_adopted_method_test {}
        return f.give() == 8384
    })
}