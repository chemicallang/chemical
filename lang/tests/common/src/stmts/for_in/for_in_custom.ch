struct ForInSpanPoint {
    var i : int

    func get_i(&self) : int {
        return i;
    }

}

variant ForInPattMatchSpan {
    Some(value : int)
    None()
}

struct span<T> {

    var _data : *T
    var _size : u64

    @implicit
    @constructor
    comptime func make2(array : []%maybe_runtime<T>) {
        return intrinsics::wrap(constructor<T>(array, intrinsics::size(array)))
    }

    @constructor
    func constructor(array_ptr : *T, array_size : u64) {
        return span<T> {
            _data : array_ptr,
            _size : array_size
        }
    }

    impl core::iterable::Linear<T> for span<T> {
        func data(&self) : *T {
            return _data;
        }
        func size(&self) : u64 {
            return _size
        }
    }

}

func test_for_in_span() {
    test("can iterate over span", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var i in s) {
            total += i
        }
        return total == 60
    })
    test("can iterate over span in reverse", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i
        }
        return total == 60
    })
    test("continue using pattern match, for in iterate over span", () => {
        var p = ForInPattMatchSpan.None()
        const arr : []int = [10, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var i, j in s) {
            if(j == 2) {
                var Some(value) = p else continue;
            }
            total += i
        }
        return total == 50
    })
    test("break using pattern match, for in iterate over span", () => {
        var p = ForInPattMatchSpan.None()
        const arr : []int = [10, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var i, j in s) {
            if(j == 2) {
                var Some(value) = p else break;
            }
            total += i
        }
        return total == 20
    })
    test("can iterate over span using a reference", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var& i in s) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over span using a reference in reverse", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var s = span<int>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over span of struct", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over span of struct in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over span of struct using a reference", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate over span of struct using a reference in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate & call method over span of struct", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over span of struct in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over span of struct using a reference", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s) {
            total += i.get_i()
        }
        return total == 70
    })
    test("can iterate & call method over span of struct using a reference in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += i.get_i()
        }
        return total == 70
    })
}