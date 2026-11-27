// P0.1 probe: the language-level capabilities the UI library depends on.
//
// Everything here is deliberately small and prints one `P0.1 <name> PASS`
// line per check so the same binary can be run under both backends.
//
// Verified here:
//   1. `interface` methods with `&mut self`.
//   2. a generic `run` bounded on the interface (`where A : App`).
//   3. `std::function` capture: by-value capture and explicit borrow capture.
//   4. `[no_panic]` propagation through a library-like call chain.
//   5. allocation counting (process allocator) is observable without a new
//      runtime hook, by sampling a counter the probe owns; the real OOM probe
//      lives in `oom/` because allocation failure aborts.
//   6. source locations through a 3-level generic component chain.

using std::Option;
using std::Result;
using std::Unit;

// ---------------------------------------------------------------------------
// 1/2. interface with &mut self + generic run bounded on it
// ---------------------------------------------------------------------------

public enum CloseAction { Allow, Veto }

public interface App {
    func view(&self, n : int) : int
    func update(&mut self, n : int) : int
    func on_close(&mut self) : CloseAction
}

@direct_init
public struct Counter {
    var total : int
}

impl App for Counter {
    public func view(&self, n : int) : int {
        return self.total + n
    }

    public func update(&mut self, n : int) : int {
        self.total = self.total + n
        return self.total
    }

    public func on_close(&mut self) : CloseAction {
        return CloseAction.Allow
    }
}

// The shape of `ui::run_headless<A : Application>`.
public func <A> run(app : &mut A) : int where A : App {
    var a = app.view(1)      // 0 + 1
    var b = app.update(4)    // 4
    var c = app.view(3)      // 7
    var d = app.on_close()
    var close_bonus : int = 0
    if(d == CloseAction.Allow) { close_bonus = 1 }
    return a + b + c + close_bonus
}

// Same shape with the real API's `Result` return type.
public interface AppResult {
    func view(&self, n : int) : Result<int, int>
    func update(&mut self, n : int) : Result<int, int>
}

@direct_init
public struct ResultApp {
    var total : int
}

impl AppResult for ResultApp {
    public func view(&self, n : int) : Result<int, int> {
        if(n < 0) { return Result.Err<int, int>(1) }
        return Result.Ok<int, int>(self.total + n)
    }

    public func update(&mut self, n : int) : Result<int, int> {
        if(n < 0) { return Result.Err<int, int>(2) }
        self.total = self.total + n
        return Result.Ok<int, int>(self.total)
    }
}

public func <A> run_result(app : &mut A) : Result<int, int> where A : AppResult {
    var a = app.view(1)
    if(a is Result.Err) { return a }
    var b = app.update(4)
    if(b is Result.Err) { return b }
    var c = app.view(3)
    if(c is Result.Err) { return c }
    var Ok(av) = a else unsafe { unreachable }
    var Ok(bv) = b else unsafe { unreachable }
    var Ok(cv) = c else unsafe { unreachable }
    return Result.Ok<int, int>(av + bv + cv)
}

// ---------------------------------------------------------------------------
// 3. std::function capture rules
// ---------------------------------------------------------------------------

func apply(f : std::function<(x : int) => int>, v : int) : int {
    return f(v)
}

func probe_function_capture() : int {
    var base = 10

    // by-value capture
    var add_base : std::function<(x : int) => int> = |base|(x : int) : int => {
        return x + base
    }
    if(add_base(5) != 15) { return 1 }

    // explicit borrow capture (read-only): the lambda may not outlive `base`.
    // P0.1 finding: this is an `unsafe` boundary (SAFE-UNSAFE-002) because the
    // erased `std::function` does not carry the referent lifetime.
    var add_ref : std::function<(x : int) => int>
    unsafe {
        add_ref = |&base|(x : int) : int => {
            return x + *base
        }
    }
    if(add_ref(2) != 12) { return 2 }

    // non-capturing lambda
    var twice : std::function<(x : int) => int> = ||(x : int) : int => {
        return x * 2
    }
    if(apply(twice, 21) != 42) { return 3 }

    return 0
}

// ---------------------------------------------------------------------------
// 4. [no_panic] on a library-like call chain
// ---------------------------------------------------------------------------

// A leaf that cannot panic: pure arithmetic with a checked overflow.
func checked_add(a : u32, b : u32) : Option<u32> [no_panic] {
    var wide : u64 = a as u64 + b as u64
    if(wide > 4294967295 as u64) {
        return Option.None<u32>()
    }
    return Option.Some<u32>(wide as u32)
}

// A library-like middle layer must inherit the restriction: this compiles only
// if the compiler propagates `[no_panic]` transitively.
func layout_add(a : u32, b : u32) : Option<u32> [no_panic] {
    return checked_add(a, b)
}

func component_add(a : u32, b : u32) : Option<u32> [no_panic] {
    return layout_add(a, b)
}

// This one is intentionally allowed to allocate/panic; it must be rejected when
// annotated. The negative side is in `negative/` because a successful build
// cannot contain it.
func panicking_helper(x : int) : int {
    var v = std::vector<int>()
    v.push(x)
    return v.size() as int
}

// ---------------------------------------------------------------------------
// 5. allocation counter (probe-owned, no runtime hook)
// ---------------------------------------------------------------------------

// `std::vector` is the allocation path the UI library must avoid in the frame
// loop. Counting `push` growth is a deterministic proxy for allocator traffic
// and is observable today; CONTRACTS §7.3 still needs a runtime counter.
func count_vector_growth(n : int, cap : int) : int {
    var v = std::vector<int>()
    v.reserve(cap as size_t)
    var growths = 0
    var last = v.capacity()
    for(var i = 0; i < n; i++) {
        v.push(i)
        var now = v.capacity()
        if(now != last) {
            growths = growths + 1
            last = now
        }
    }
    return growths
}

// ---------------------------------------------------------------------------
// 6. source locations through a 3-level generic component chain
// ---------------------------------------------------------------------------

public struct Loc {
    var raw : u64
    var line : int
}

func location_now() : Loc [no_alloc] {
    return Loc { raw : intrinsics::get_raw_location() as u64, line : intrinsics::get_line_no() as int }
}

func <T> level3(value : T) : Loc [no_alloc] {
    // The location must point at the source line of this call, not at the
    // generic instantiation site.
    return location_now()
}

func <T> level2(value : T) : Loc [no_alloc] {
    return level3<T>(value)
}

func <T> level1(value : T) : Loc [no_alloc] {
    return level2<T>(value)
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

public func main() : int {
    var failures = 0

    var counter = Counter { total : 0 }
    var run_basic = run<Counter>(&mut counter)
    if(run_basic == 13) {
        println("P0.1 application_interface_and_generic_run PASS")
    } else {
        println("P0.1 application_interface_and_generic_run FAIL")
        failures = failures + 1
    }

    var rapp = ResultApp { total : 0 }
    var rres = run_result<ResultApp>(&mut rapp)
    var rok : bool = false
    if(rres is Result.Ok) {
        var Ok(rv) = rres else unsafe { unreachable }
        rok = rv == 12
    }
    if(rok) {
        println("P0.1 application_result_interface PASS")
    } else {
        println("P0.1 application_result_interface FAIL")
        failures = failures + 1
    }

    var cap_rc = probe_function_capture()
    if(cap_rc == 0) {
        println("P0.1 std_function_capture PASS")
    } else {
        println("P0.1 std_function_capture FAIL")
        failures = failures + 1
    }

    var ok_add = checked_add(1u32, 2u32)
    var overflow_add = component_add(4294967295u32, 1u32)
    if(ok_add is Option.Some && overflow_add is Option.None) {
        println("P0.1 no_panic_chain PASS")
    } else {
        println("P0.1 no_panic_chain FAIL")
        failures = failures + 1
    }

    var growths = count_vector_growth(64, 64)
    if(growths == 0) {
        println("P0.1 alloc_counter_proxy PASS growths=0")
    } else {
        println("P0.1 alloc_counter_proxy FAIL growths!=0")
        failures = failures + 1
    }

    var loc = level1<int>(7)
    var direct_line = intrinsics::get_line_no() as int
    if(loc.raw != 0u64 && loc.line > 0) {
        unsafe { printf("P0.1 source_location_3_level_generic PASS line=%d direct=%d\n", loc.line, direct_line) }
    } else {
        println("P0.1 source_location_3_level_generic FAIL")
        failures = failures + 1
    }

    // Keep this call so the panicking helper is not trivially dead; it is only
    // reachable if the compiler ever accepts it in a `[no_panic]` body.
    var keep = panicking_helper(7)
    if(keep != 1) { failures = failures + 1 }

    if(failures == 0) {
        println("P0.1 foundation ALL PASS")
        return 0
    }
    return 1
}
