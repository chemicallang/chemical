// ============================================================
// Destructor ownership for a by-value aggregate temporary passed
// to a *capturing function* call.
//
// Regression: a call through a capturing function (the `%capture`
// type behind `std::function<...>`) is lowered by
// `MIRLowerer::lower_capturing_function_call`, which resolves the
// callee's parameter types but did not pass them down to
// `lower_call_args_for`. With an unknown parameter type the lowerer
// conservatively treats every temporary argument as *caller*-owned
// and destroys it after the call — while the callee, whose aggregate
// parameter is materialised as its own by-value copy, destroys that
// copy too. A destructible temporary argument was therefore destroyed
// twice (a double free of whatever the destructor released, e.g. the
// heap buffers of an `http::Request` handed to a route handler).
//
// `http::server::handle_conn` hit exactly this: it called
// `route.handler(req_opt.take(), resw)` — `route.handler` is a
// `std::function` and the `req_opt.take()` temporary is a destructible
// aggregate — and every request tore the request down twice. The
// workaround was to hoist the `take()` into a local (`route.handler(req,
// resw)`) so the argument was an identifier instead of a temporary.
//
// The test below needs no standard library: it declares a minimal
// capturing-function instance type locally (`%capture<(d : Destructible)
// => void, ...>`) and registers a no-capture lambda in it, which is all
// the call site needs.
// ============================================================

struct CAP_Destructible {

    var data : int

    var count : *mut int

    var lamb : (count : *mut int) => void

    @delete
    func delete(&self) {
        self.lamb(self.count);
    }

}

func cap_create_destructible(count : *mut int, data : int) : CAP_Destructible { unsafe {
    return CAP_Destructible {
        data : data,
        count : count,
        lamb : (count : *mut int) => {
            *count = *count + 1;
        }
    }
 }}

// ─────────────────────────────────────────────────────────────
// Minimal capturing-function instance (no standard library
// dependency): a fn pointer + a captured-data pointer, plus the
// `get_fn_ptr` / `get_data_ptr` protocol the lowerer looks for.
// ─────────────────────────────────────────────────────────────

@retained
struct CAP_FnInstance {

    var fn_pointer : *mut any

    var fn_data_ptr : *mut any

    @maxalign
    var buffer : [32]char

    @make
    @expand
    comptime func make(lambda : () => void) {
        const ptr = intrinsics::get_lambda_fn_ptr(lambda)
        const cap = intrinsics::get_lambda_cap_ptr(lambda)
        return make2(ptr, cap)
    }

    @make
    func make2(ptr : *mut void, cap : *mut void) {
        return CAP_FnInstance {
            fn_pointer : ptr,
            fn_data_ptr : cap,
            buffer : []
        }
    }

    func get_fn_ptr(&self) : *mut any {
        return fn_pointer
    }

    func get_data_ptr(&self) : *mut any {
        if(fn_data_ptr != null) {
            return fn_data_ptr
        }
        return (&buffer[0]) as *mut any
    }

}

// the effectful callback type used below, and a holder so that the
// receiver is reached as a struct member (as `route.handler` is)
type CAP_Callback = %capture<(d : CAP_Destructible) => void, CAP_FnInstance>

struct CAP_Route {
    var handler : CAP_Callback
}

func cap_invoke_with_temporary(route : *CAP_Route, count : *mut int) {
    unsafe { route.handler(cap_create_destructible(count, 334)) }
}

// A control: the same call, but with the argument in a local first.
func cap_invoke_with_local(route : *CAP_Route, count : *mut int) {
    unsafe {
        var d = cap_create_destructible(count, 556)
        route.handler(d)
    }
}

func test_capturing_call_destruction() {
    test("capturing function call destroys a temporary argument exactly once", () => {
        comptime if(intrinsics::is_interpretation()) {
            return true;
        } else {
            var count = 0
            var h : CAP_Callback = ||(d) => {
                return;
            }
            var route = CAP_Route { handler : h }
            cap_invoke_with_temporary(&raw mut route, &raw mut count)
            return count == 1
        }
    })
    test("capturing function call destroys a local argument exactly once", () => {
        comptime if(intrinsics::is_interpretation()) {
            return true;
        } else {
            var count = 0
            var h : CAP_Callback = ||(d) => {
                return;
            }
            var route = CAP_Route { handler : h }
            cap_invoke_with_local(&raw mut route, &raw mut count)
            return count == 1
        }
    })
}
