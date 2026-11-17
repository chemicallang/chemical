// JS backend — `for-in` over a **custom** `Iterable`/`ReversibleIterable`.
//
// `for-in` over std's own containers is covered by the std fixture; what was
// untested is the same lowering over a user implementation, which goes through
// interface calls (`begin`/`valid`/`current`/`next`) that the JS backend
// dispatches by function address. Identical stdout from both backends pins that
// path — including `current()` returning `&T` (a reference read through the
// cursor) and `reversed`, which adds `rbegin`/`previous`/`count`.
//
// The cursor is a pointer into the collection's own storage, so it stays valid
// across `next`: that is the shape the interface is designed for, and the one a
// user collection actually has.

using core::iterable::Iterable;
using core::iterable::ReversibleIterable;

public struct IntArray {
    var data : [4]int
    var len : int
}

impl Iterable<int, *int> for IntArray {
    func begin(&self) : *int {
        unsafe { return &raw self.data[0] }
    }

    // Both bounds, so the same predicate is correct forwards *and* backwards: a
    // `reversed` cursor walks below the first element, and an upper-bound-only
    // check would stay true forever (which is how this fixture first hung).
    func valid(&self, it : *int) : bool {
        unsafe {
            return it >= &raw self.data[0] && it < &raw self.data[len]
        }
    }

    func current(&self, it : *int) : &int {
        unsafe { return unsafe(it) as &int }
    }

    func next(&self, it : *int) : *int {
        unsafe { return it + 1 }
    }
}

impl ReversibleIterable<int, *int> for IntArray {
    func rbegin(&self) : *int {
        unsafe { return &raw self.data[len - 1] }
    }

    func previous(&self, it : *int) : *int {
        unsafe { return it - 1 }
    }

    func count(&self) : u64 {
        return len as u64
    }
}

public func main() : int {
    unsafe {
        var coll = IntArray { data: [3, 4, 5, 6], len: 4 }

        var sum : int = 0
        for(var v in coll) {
            printf("f=%d\n", v)
            sum = sum + v
        }
        printf("forward_sum=%d\n", sum)

        var acc : int = 0
        for(var v in coll reversed) {
            printf("r=%d\n", v)
            acc = acc * 10 + v
        }
        printf("reverse_number=%d\n", acc)

        return 0
    }
}
