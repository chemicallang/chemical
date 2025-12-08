impl PrimitiveImplInterface1 for int {
    func give(&self) : int {
        return *self + 10
    }
}

interface PrimitiveImplInterface1 {
    func give(&self) : int
}

func <T : PrimitiveImplInterface1> prim_impl_interface1_give_stat_dispatch(value : T) : int {
    return value.give()
}

impl PrimitiveImplInterface1 for long {
    func give(&self) : int {
        return (*self + 20) as int
    }
}

impl PrimitiveImplInterface1 for double {
    func give(&self) : int {
        return *self as int
    }
}

impl PrimitiveImplInterface1 for float {
    func give(&self) : int {
        return (*self + 1.0f) as int
    }
}

impl PrimitiveImplInterface1 for bool {
    func give(&self) : int {
        return if(*self) 69 else 96
    }
}

interface PrimitiveImplInterfaceRef1 {
    func give_ref(&self) : int
}

impl PrimitiveImplInterfaceRef1 for &int {
    func give_ref(&self) : int {
        return **self + 30
    }
}

impl PrimitiveImplInterfaceRef1 for &long {
    func give_ref(&self) : int {
        return (**self + 40) as int
    }
}

impl PrimitiveImplInterfaceRef1 for *int {
    func give_ref(&self) : int {
        return **self + 50
    }
}

impl PrimitiveImplInterfaceRef1 for *long {
    func give_ref(&self) : int {
        return (**self + 60) as int
    }
}

func param_call_int_prim_impl(a : int) : int {
    return a.give()
}

func param_call_long_prim_impl(a : long) : int {
    return a.give()
}

func param_call_ref_int_prim_impl(a : &int) : int {
    return a.give_ref()
}

func param_call_ref_long_prim_impl(a : &long) : int {
    return a.give_ref()
}

func param_call_ptr_int_prim_impl(a : *int) : int {
    return a.give_ref()
}

func param_call_ptr_long_prim_impl(a : *long) : int {
    return a.give_ref()
}

func dyn_prim_impl_give(i : dyn PrimitiveImplInterface1) : int {
    return i.give()
}

@static
interface PrimitiveImplInterface2 {
    func curr_mod_give(&self) : int
}

impl PrimitiveImplInterface2 for int {
    func curr_mod_give(&self) : int {
        return *self + 3;
    }
}

impl PrimitiveImplInterface3 for int {
    func curr_mod_give2(&self) : int {
        return *self + 7;
    }
}

func test_primitive_implementations() {
    test("direct call on primitive impl method works - 1", () => {
        var i : int = 234
        return i.give() == 244
    })
    test("direct call on primitive impl method works - 2", () => {
        var i : long = 61
        return i.give() == 81
    })
    test("static dispatch on primitive impl in current module method works - 1", () => {
        var i : int = 234
        return prim_impl_interface1_give_stat_dispatch(i) == 244
    })
    test("static dispatch on primitive impl in current module method works - 2", () => {
        var i : long = 61
        return prim_impl_interface1_give_stat_dispatch(i) == 81
    })
    test("call through parameter, primitive impl method works - 1", () => {
        return param_call_int_prim_impl(10) == 20
    })
    test("call through parameter, primitive impl method works - 2", () => {
        return param_call_long_prim_impl(10) == 30
    })
    test("call on referenced primitive impl method works - 1", () => {
        var i : int = 20
        return param_call_ref_int_prim_impl(i) == 50
    })
    test("call on referenced primitive impl method works - 2", () => {
        var i : long = 20
        return param_call_ref_long_prim_impl(i) == 60
    })
    test("direct call on pointer primitive impl method works - 1", () => {
        var i : int = 20
        var j = &i
        return j.give_ref() == 70
    })
    test("direct call on pointer primitive impl method works - 2", () => {
        var i : long = 20
        var j = &i
        return j.give_ref() == 80
    })
    test("call through parameter on pointer primitive impl method works - 1", () => {
        var i : int = 20
        return param_call_ptr_int_prim_impl(&i) == 70
    })
    test("call through parameter on pointer primitive impl method works - 2", () => {
        var i : long = 20
        return param_call_ptr_long_prim_impl(&i) == 80
    })
    test("direct call on primitive impl method works - 3", () => {
        var i : double = 3.0
        return i.give() == 3
    })
    test("direct call on primitive impl method works - 4", () => {
        var i : float = 3.0f
        return i.give() == 4
    })
    test("direct call on primitive impl method works - 5", () => {
        var i : bool = true
        return i.give() == 69
    })
    test("direct call on primitive impl method works - 6", () => {
        var i : bool = false
        return i.give() == 96
    })
    test("external direct call on primitive impl method works - 1", () => {
        var i : int = 234
        return i.ext_give() == 244
    })
    test("external direct call on primitive impl method works - 2", () => {
        var i : long = 61
        return i.ext_give() == 81
    })
    test("external call through parameter, primitive impl method works - 1", () => {
        return ext_param_call_int_prim_impl(10) == 20
    })
    test("external call through parameter, primitive impl method works - 2", () => {
        return ext_param_call_long_prim_impl(10) == 30
    })
    test("static dispatch on primitive impl in external module method works - 1", () => {
        var i : int = 234
        return ext_prim_impl_interface1_give_stat_dispatch(i) == 244
    })
    test("static dispatch on primitive impl in external module method works - 2", () => {
        var i : long = 61
        return ext_prim_impl_interface1_give_stat_dispatch(i) == 81
    })
    test("external call on referenced primitive impl method works - 1", () => {
        var i : int = 20
        return ext_param_call_ref_int_prim_impl(i) == 50
    })
    test("external call on referenced primitive impl method works - 2", () => {
        var i : long = 20
        return ext_param_call_ref_long_prim_impl(i) == 60
    })
    test("external direct call on pointer primitive impl method works - 1", () => {
        var i : int = 20
        var j = &i
        return j.ext_give_ref() == 70
    })
    test("external direct call on pointer primitive impl method works - 2", () => {
        var i : long = 20
        var j = &i
        return j.ext_give_ref() == 80
    })
    test("external call through parameter on pointer primitive impl method works - 1", () => {
        var i : int = 20
        return ext_param_call_ptr_int_prim_impl(&i) == 70
    })
    test("external call through parameter on pointer primitive impl method works - 2", () => {
        var i : long = 20
        return ext_param_call_ptr_long_prim_impl(&i) == 80
    })
    test("external direct call on primitive impl method works - 3", () => {
        var i : double = 3.0
        return i.ext_give() == 3
    })
    test("external direct call on primitive impl method works - 4", () => {
        var i : float = 3.0f
        return i.ext_give() == 4
    })
    test("external direct call on primitive impl method works - 5", () => {
        var i : bool = true
        return i.ext_give() == 69
    })
    test("external direct call on primitive impl method works - 6", () => {
        var i : bool = false
        return i.ext_give() == 96
    })
    test("direct call on primitive impl method works - 1", () => {
        var i : int = 234
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 244
    })
    test("dynamic call on primitive impl method works - 2", () => {
        var i : long = 61
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 81
    })
    test("dynamic call on primitive impl method works - 3", () => {
        var i : double = 3.0
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 3
    })
    test("dynamic call on primitive impl method works - 4", () => {
        var i : float = 3.0f
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 4
    })
    test("dynamic call on primitive impl method works - 5", () => {
        var i : bool = true
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 69
    })
    test("dynamic call on primitive impl method works - 6", () => {
        var i : bool = false
        return dyn_prim_impl_give(dyn<PrimitiveImplInterface1>(i)) == 96
    })
    test("primitive impl on static interface in current module works", () => {
        var i : int = 343
        return i.curr_mod_give() == 346
    })
    test("primitive impl on static interface from external module works", () => {
        var i : int = 233
        return i.curr_mod_give2() == 240
    })
}