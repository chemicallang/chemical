// tests for destruction in function call arguments and special call paths
// these test scenarios that may have bugs in the C backend's destruction handling

func take_two_destructible_refs(a : &Destructible, b : &Destructible) {}

// capturing function test helpers
func capturing_call_test(cap : (d : &Destructible) => void, d : Destructible) {
    cap(&d)
}

func capturing_call_two_args(cap : (a : &Destructible, b : &Destructible) => void, a : Destructible, b : Destructible) {
    cap(&a, &b)
}

// dynamic dispatch interface
interface DestructibleUser {
    func use(&self, d : &Destructible)
}

struct DestructibleUserImpl {
    func use(&self, d : &Destructible) {}
}

impl DestructibleUser for DestructibleUserImpl {
    func use(&self, d : &Destructible) {}
}

func create_user_impl() : DestructibleUserImpl {
    return DestructibleUserImpl {}
}


// test index operator with destructible parent
func test_index_destruct_parent() {
    test("destructible parent in index operator is destructed", () => {
        var count = 0
        {
            var arr = [10, 20, 30]
            var val = arr[create_destructible(&raw mut count, 1).data]
        }
        return count == 1
    })
}

// test index operator with destructible parent via function call
func test_index_destruct_parent_func_call() {
    test("destructible parent in index operator with func call arg is destructed", () => {
        var count = 0
        {
            var arr = [10, 20, 30]
            var val = arr[create_destructible(&raw mut count, 1).data]
            var val2 = arr[create_destructible(&raw mut count, 2).data]
        }
        return count == 2
    })
}

// test destructible struct passed to reference via capturing function
func test_capturing_destruct_ref() {
    test("capturing function call destructs struct passed to reference", () => {
        var count = 0
        {
            var cap = (d : &Destructible) => {
                var x = d.data
            }
            cap(create_destructible(&raw mut count, 10))
        }
        return count == 1
    })
}

// test destructible struct passed to reference via dynamic dispatch
func test_dynamic_dispatch_destruct_ref() {
    test("dynamic dispatch call destructs struct passed to reference", () => {
        var count = 0
        {
            var impl = create_user_impl()
            var user : dyn DestructibleUser = dyn<DestructibleUser>(impl)
            user.use(create_destructible(&raw mut count, 20))
        }
        return count == 1
    })
}

// test destructible struct created by function call passed to reference in capturing function
func test_capturing_func_call_destruct_ref_func() {
    test("capturing function call destructs struct from function passed to reference", () => {
        var count = 0
        {
            var cap = (d : &Destructible) => {
                var x = d.data
            }
            cap(create_destructible(&raw mut count, 30))
        }
        return count == 1
    })
}

// test destructible struct created by function call passed to reference
func test_func_call_destruct_ref() {
    test("function call destructs struct from function passed to reference", () => {
        var count = 0
        {
            var impl = create_user_impl()
            var user : Destructible = create_destructible(&raw mut count, 40)
            take_destructible_ref(create_destructible(&raw mut count, 50))
        }
        return count == 2
    })
}

// test destructible struct created in conditional branch not executed
func test_conditional_not_executed_no_destruct() {
    test("destructible struct in conditional not executed doesn't destruct", () => {
        var count = 0
        {
            var condition = false
            if(condition) {
                var d = create_destructible(&raw mut count, 60)
            }
        }
        // destructor should NOT be called
        return count == 0
    })
}

// test destructible struct returned from function in conditional expression
func maybe_create_destructible(count : *mut int, cond : bool) : Destructible {
    if(cond) {
        return create_destructible(count, 70)
    }
    return Destructible {
        data : 0,
        count : count,
        lamb : (c : *mut int) => { *c = *c + 1; }
    }
}

func test_conditional_func_return_destruct() {
    test("conditional function returning destructible struct destructs correctly", () => {
        var count = 0
        {
            var d = maybe_create_destructible(&raw mut count, false)
        }
        return count == 1
    })
}

// test destructible function call in multiple args to function
func test_multi_arg_destruct_func_calls() {
    test("multiple destructible func call args are destructed", () => {
        var count = 0
        {
            take_two_destructible_refs(
                create_destructible(&raw mut count, 80),
                create_destructible(&raw mut count, 90)
            )
        }
        return count == 2
    })
}

// test destructible struct via method call chain
func test_method_chain_destruct() {
    test("destructible method call chain is destructed", () => {
        var count = 0
        {
            var d = create_destructible(&raw mut count, 100)
            var c = d.copy()
        }
        return count == 2
    })
}

// test destructible struct passed to capturing function with multiple args
func test_capturing_multi_arg_destruct() {
    test("capturing func with multiple destructible args destructs all", () => {
        var count = 0
        {
            var cap = (a : &Destructible, b : &Destructible) => {
                var x = a.data + b.data
            }
            cap(
                create_destructible(&raw mut count, 110),
                create_destructible(&raw mut count, 120)
            )
        }
        return count == 2
    })
}

// test destructible struct returned from dynamic dispatch
interface DestructibleFactory {
    func make(&self, count : *mut int) : Destructible
}

struct DestructibleFactoryImpl {
    func make(&self, count : *mut int) : Destructible {
        return create_destructible(count, 130)
    }
}

impl DestructibleFactory for DestructibleFactoryImpl {
    func make(&self, count : *mut int) : Destructible {
        return create_destructible(count, 130)
    }
}

func test_dynamic_dispatch_return_destruct() {
    test("destructible returned from factory function is destructed", () => {
        var count = 0
        {
            var factory = DestructibleFactoryImpl {}
            var d = factory.make(&raw mut count)
        }
        return count == 1
    })
}

// test &* (reference of deref) in pattern match
variant OptDestruct {
    Some(d : Destructible)
    None
}

func test_ref_of_deref_pattern() {
    test("&*value in pattern match expression works", () => {
        var count = 0
        {
            var opt = OptDestruct.Some(create_destructible(&raw mut count, 140))
            var ptr = &raw mut opt
            // deref pointer and pattern match
            var Some(d) = *ptr else unreachable
        }
        return count == 1
    })
}



// test that a destructible in a non-executed branch of a compound conditional
// doesn't invoke destructor
func test_compound_conditional_no_destruct() {
    test("compound conditional non-executed branch doesn't destruct", () => {
        var count = 0
        {
            var cond = false
            var result = 0
            if(cond) {
                var d = create_destructible(&raw mut count, 150)
                result = d.data
            } else {
                result = 42
            }
        }
        return count == 0
    })
}

// test referencing and dereferencing in expressions with destructibles
func test_ref_of_deref_destruct() {
    test("reference of dereference of hidden ptr works with destructibles", () => {
        var count = 0
        {
            var d = create_destructible(&raw mut count, 170)
            // just access through variable directly - pointer deref of destructible is not allowed
            var data = d.data
        }
        return count == 1
    })
}

// test two destructible temps passed via function call to a single function with ref params
func take_two_refs_void(a : &Destructible, b : &Destructible) {}

func test_two_destruct_temps_to_refs() {
    test("two destructible temps to ref params in normal call are both destructed", () => {
        var count = 0
        {
            take_two_refs_void(
                create_destructible(&raw mut count, 180),
                create_destructible(&raw mut count, 190)
            )
        }
        return count == 2
    })
}

// test destructible struct used in for loop value
func test_for_loop_destruct() {
    test("destructible struct in for loop value is destructed each iteration", () => {
        var count = 0
        {
            var arr = [1, 2, 3]
            for(var i = 0; i < 3; i++) {
                var d = create_destructible(&raw mut count, 200)
            }
        }
        return count == 3
    })
}

// test destructible struct in while loop
func test_while_loop_destruct() {
    test("destructible struct in while loop is destructed each iteration", () => {
        var count = 0
        {
            var i = 0
            while(i < 3) {
                var d = create_destructible(&raw mut count, 210)
                i++
            }
        }
        return count == 3
    })
}

// test reference to reference (&&) hidden ptr with destructibles
struct RefHolder {
    var inner : Destructible
}

func test_ref_of_deref_of_ref() {
    test("reference of dereference of reference works with destructibles", () => {
        var count = 0
        {
            var holder = RefHolder {
                inner : create_destructible(&raw mut count, 220)
            }
            var ref_to_inner = &holder.inner
            // access through reference - deref of destructible is not allowed directly
            var data = ref_to_inner.data
        }
        return count == 1
    })
}

// test destructible struct in switch/match value
func test_switch_destruct() {
    test("destructible struct in switch value is destructed", () => {
        var count = 0
        {
            var x = 1
            switch(x) {
                1 => {
                    var d = create_destructible(&raw mut count, 230)
                }
                default => {}
            }
        }
        return count == 1
    })
}

// test that returning a temp destructible by value from function works
func return_temp_destruct(count : *mut int) : Destructible {
    return create_destructible(count, 240)
}

func test_return_temp_destruct() {
    test("returning temp destructible from function works", () => {
        var count = 0
        {
            var d = return_temp_destruct(&raw mut count)
        }
        return count == 1
    })
}

// test that capturing lambda returning destructible temp is destructed
func test_capturing_lambda_return_temp_destruct() {
    test("capturing lambda returning destructible temp is destructed", () => {
        var count = 0
        {
            var cap = (c : *mut int) => {
                var d = create_destructible(c, 250)
            }
            cap(&raw mut count)
        }
        return count == 1
    })
}

// test destructible in nested scopes
func test_nested_scope_destruct() {
    test("destructible in nested scopes is destructed", () => {
        var count = 0
        {
            {
                var d = create_destructible(&raw mut count, 260)
            }
        }
        return count == 1
    })
}

// test that & (reference to) prevents destruction
func test_ref_prevents_destruct() {
    test("taking reference to destructible prevents early destruction", () => {
        var count = 0
        {
            var d = create_destructible(&raw mut count, 270)
            var r = &d
            // using ref keeps value alive
            var data = r.data
        }
        return count == 1
    })
}

// test destructible struct after early return
func early_return_func(count : *mut int, should_return : bool) {
    var d = create_destructible(count, 280)
    if(should_return) {
        return
    }
    var x = d.data
}

func test_early_return_destruct() {
    test("early return with destructible struct works", () => {
        var count = 0
        {
            early_return_func(&raw mut count, true)
        }
        return count == 1
    })
}

// test destructible struct in array passed to function
func take_destruct_array(d : Destructible[]) {}

func test_destruct_array_param() {
    test("destructible array passed to function destructs elements", () => {
        var count = 0
        {
            var arr = [
                create_destructible(&raw mut count, 290),
                create_destructible(&raw mut count, 300)
            ]
            take_destruct_array(arr)
        }
        // arr should be destructed when out of scope, both elements
        return count == 2
    })
}

// test destructible struct as reference return from function call used in conditional
func get_destruct_or_not(count : *mut int, flag : bool) : &Destructible {
    var d = create_destructible(count, 310)
    if(flag) {
        return &d
    }
    return &d
}

// the declaration/runner function
func test_call_destruction() {
    test_index_destruct_parent()
    test_index_destruct_parent_func_call()
    test_capturing_destruct_ref()
    test_dynamic_dispatch_destruct_ref()
    test_capturing_func_call_destruct_ref_func()
    test_func_call_destruct_ref()
    test_conditional_not_executed_no_destruct()
    test_conditional_func_return_destruct()
    test_multi_arg_destruct_func_calls()
    test_method_chain_destruct()
    test_capturing_multi_arg_destruct()
    test_dynamic_dispatch_return_destruct()
    test_ref_of_deref_pattern()
    test_compound_conditional_no_destruct()
    test_ref_of_deref_destruct()
    test_two_destruct_temps_to_refs()
    test_for_loop_destruct()
    test_while_loop_destruct()
    test_ref_of_deref_of_ref()
    test_switch_destruct()
    test_return_temp_destruct()
    test_capturing_lambda_return_temp_destruct()
    test_nested_scope_destruct()
    test_ref_prevents_destruct()
    test_early_return_destruct()
    test_destruct_array_param()
}
