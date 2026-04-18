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
}