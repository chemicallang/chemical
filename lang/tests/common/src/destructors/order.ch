// Destruction order is normative (mir-design.md §7.4): local/parameter scopes
// are destroyed in reverse initialization order, fixed arrays last-to-first, and
// struct members in declaration order. The C backend (MIR) used to emit
// forward/declaration order for scope and function-end cleanup, which is only
// observable through destructor side effects like the log below.

// The destructor records its id into a base-100 sequence, so the order is
// encoded in a single integer (array writes from a destructor are awkward in
// interpretation mode). e.g. ids 3,2,1 -> 3*10000 + 2*100 + 1 == 30201.
var dtor_order_seq = 0

struct OrderProbe {
    var id : int

    @delete
    func delete(&self) {
        dtor_order_seq = dtor_order_seq * 100 + id
    }
}

func order_three_locals() {
    var a = OrderProbe { id : 1 }
    var b = OrderProbe { id : 2 }
    var c = OrderProbe { id : 3 }
}

func order_early_return(early : bool) {
    var a = OrderProbe { id : 10 }
    if(early) { return }
    var b = OrderProbe { id : 20 }
}

func order_param(p : OrderProbe, id : int) {
    var local = OrderProbe { id : id }
}

struct OrderHolder {
    var a : OrderProbe
    var b : OrderProbe
}

func test_destructor_order() {

    test("locals are destroyed in reverse initialization order", () => {
        dtor_order_seq = 0
        order_three_locals()
        return dtor_order_seq == 30201
    })

    test("early return destroys locals in reverse order", () => {
        dtor_order_seq = 0
        order_early_return(true)
        return dtor_order_seq == 10
    })

    test("fallthrough destroys locals in reverse order", () => {
        dtor_order_seq = 0
        order_early_return(false)
        return dtor_order_seq == 2010
    })

    test("fixed array elements are destroyed last-to-first", () => {
        dtor_order_seq = 0
        {
            var arr : [4]OrderProbe = [
                OrderProbe { id : 1 },
                OrderProbe { id : 2 },
                OrderProbe { id : 3 },
                OrderProbe { id : 4 }
            ]
        }
        return dtor_order_seq == 4030201
    })

    test("struct members are destroyed in declaration order", () => {
        dtor_order_seq = 0
        {
            var h = OrderHolder { a : OrderProbe { id : 1 }, b : OrderProbe { id : 2 } }
        }
        return dtor_order_seq == 102
    })

    test("nested scopes destroy the inner locals first", () => {
        dtor_order_seq = 0
        {
            var a = OrderProbe { id : 1 }
            {
                var b = OrderProbe { id : 2 }
            }
            var c = OrderProbe { id : 3 }
        }
        return dtor_order_seq == 20301
    })

    test("function parameters are destroyed after locals", () => {
        dtor_order_seq = 0
        order_param(OrderProbe { id : 1 }, 2)
        return dtor_order_seq == 201
    })

    test("break destroys the loop-local exactly once", () => {
        dtor_order_seq = 0
        for(var i = 0; i < 10; i++) {
            var d = OrderProbe { id : 900 + i }
            if(i == 2) { break }
        }
        return dtor_order_seq == 9091002
    })
}
