// JS backend — closures (Phase 1, step 2).
//
// A capturing lambda stored in `std::function` is the shape real client code
// uses most (callbacks, event handlers, `for-in` bodies). The native and JS
// backends must agree on it, which pins the fat-pointer ABI: a call through a
// function value passes the capture environment as the first argument, and a
// function address is a number that survives a round trip through memory.

func apply_twice(f : std::function<(x : int) => int>, v : int) : int {
    return f(f(v))
}

public func main() : int {
    unsafe {
        var base : int = 10
        var add_base : std::function<(x : int) => int> = |base|(x : int) : int => {
            return base + x
        }
        printf("closure=%d\n", add_base(5))

        // two closures over different captures, same generated function
        var other : int = 100
        var add_other : std::function<(x : int) => int> = |other|(x : int) : int => {
            return other + x
        }
        printf("closure2=%d\n", add_other(7))

        // a non-capturing lambda still goes through the same call shape
        var twice : std::function<(x : int) => int> = ||(v : int) : int => {
            return v * 2
        }
        printf("plain=%d\n", twice(21))

        // a closure passed through a function parameter (aggregate by pointer)
        printf("apply=%d\n", apply_twice(add_base, 3))

        // a capture that owns heap memory: the environment is copied in and
        // dropped with the closure
        var prefix = std::string("hello")
        var prefix_len : std::function<(x : int) => int> = |prefix|(v : int) : int => {
            return v + (prefix.size() as int)
        }
        printf("capture_string=%d\n", prefix_len(5))

        // for-in over a vector: iterator machinery from std, not a closure
        var vals = std::vector<int>()
        vals.push_back(3)
        vals.push_back(4)
        vals.push_back(5)
        var total : int = 0
        for(var v in vals) {
            total = total + v
        }
        printf("forin=%d\n", total)
    }
    return 0
}
