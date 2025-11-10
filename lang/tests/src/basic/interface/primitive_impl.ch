interface PrimitiveImplInterface1 {
    func give(&self) : int
}

impl PrimitiveImplInterface1 for int {
    func give(&self) : int {
        return *self + 10
    }
}

impl PrimitiveImplInterface1 for long {
    func give(&self) : int {
        return (*self + 20) as int
    }
}

impl PrimitiveImplInterface1 for &int {
    func give(&self) : int {
        return **self + 30
    }
}

impl PrimitiveImplInterface1 for &long {
    func give(&self) : int {
        return (**self + 40) as int
    }
}

impl PrimitiveImplInterface1 for *int {
    func give(&self) : int {
        return **self + 50
    }
}

impl PrimitiveImplInterface1 for *long {
    func give(&self) : int {
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
    return a.give()
}

func param_call_ref_long_prim_impl(a : &long) : int {
    return a.give()
}

func param_call_ptr_int_prim_impl(a : *int) : int {
    return a.give()
}

func param_call_ptr_long_prim_impl(a : *long) : int {
    return a.give()
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
    test("call through parameter, primitive impl method works - 1", () => {
        return param_call_int_prim_impl(10) == 20
    })
    test("call through parameter, primitive impl method works - 2", () => {
        return param_call_long_prim_impl(10) == 30
    })
    // TODO:
    // test("call on referenced primitive impl method works - 1", () => {
    //     var i : int = 20
    //     return param_call_ref_int_prim_impl(i) == 50
    // })
    // TODO:
    // test("call on referenced primitive impl method works - 2", () => {
    //     var i : long = 20
    //     return param_call_ref_long_prim_impl(i) == 60
    // })
    test("direct call on pointer primitive impl method works - 1", () => {
        var i : int = 20
        var j = &i
        return j.give() == 70
    })
    test("direct call on pointer primitive impl method works - 2", () => {
        var i : long = 20
        var j = &i
        return j.give() == 80
    })
    // TODO:
    // test("call through parameter on pointer primitive impl method works - 1", () => {
    //     var i : int = 20
    //     return param_call_ptr_int_prim_impl(&i) == 70
    // })
    // TODO:
    // test("call through parameter on pointer primitive impl method works - 2", () => {
    //     var i : long = 20
    //     return param_call_ptr_long_prim_impl(&i) == 80
    // })
}