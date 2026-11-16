// JS backend — first runnable program (Phase 1).
//
// The native backend and the JS backend must print identical output; that is the
// differential gate in scripts/js-backend-test.sh. Keep this program to the
// subset the emitter claims to support today: integers, loops, calls, and
// printf (mapped onto the JS runtime). Extend it as the emitter grows.

func add(a : int, b : int) : int {
    return a + b
}

func fib(n : int) : int {
    if(n < 2) {
        return n
    } else {
        return fib(n - 1) + fib(n - 2)
    }
}

public func main() : int {
    unsafe {
        var sum : int = 0
        var i : int = 1
        while(i <= 5) {
            sum = add(sum, i)
            i = i + 1
        }
        printf("sum=%d\n", sum)

        // signed / unsigned arithmetic and comparison
        var small : i8 = 127
        small = small + 1
        printf("i8_wrap=%d\n", small as int)
        var big : u32 = 4294967295u
        big = big + 1u
        printf("u32_wrap=%.0f\n", big as double)
        printf("div=-7/2=%d rem=-7%%2=%d\n", -7 / 2, -7 % 2)

        // a real recursion with a branch
        printf("fib10=%d\n", fib(10))

        // pointers into the frame, and a struct through a pointer
        var box : int = 41
        var ptr : *mut int = &raw mut box
        ptr[0] = ptr[0] + 1
        printf("ptr=%d\n", box)

        printf("i=%d sum_plus_i=%d\n", i, add(sum, i))
    }
    return 0
}
