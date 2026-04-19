struct ForInSpanPoint {
    var i : int

    func get_i(&self) : int {
        return i;
    }

}

func test_for_in_span() {
    test("can iterate over span", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var s = std::span<int>(arr)
        var total = 0;
        for(var i in s) {
            total += i
        }
        return total == 60
    })
    test("can iterate over span in reverse", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var s = std::span<int>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i
        }
        return total == 60
    })
    test("can iterate over span using a reference", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var s = std::span<int>(arr)
        var total = 0;
        for(var& i in s) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over span using a reference in reverse", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var s = std::span<int>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over span of struct", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over span of struct in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over span of struct using a reference", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate over span of struct using a reference in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate & call method over span of struct", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over span of struct in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 5 }, ForInSpanPoint { i : 15 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var i in s reversed) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over span of struct using a reference", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s) {
            total += i.get_i()
        }
        return total == 70
    })
    test("can iterate & call method over span of struct using a reference in reverse", () => {
        const arr : []ForInSpanPoint = [ ForInSpanPoint { i : 20 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 10 }, ForInSpanPoint { i : 20 }]
        var s = std::span<ForInSpanPoint>(arr)
        var total = 0;
        for(var& i in s reversed) {
            total += i.get_i()
        }
        return total == 70
    })
}