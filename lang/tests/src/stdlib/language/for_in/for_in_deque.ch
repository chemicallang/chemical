func test_for_in_deque() {
    test("can iterate over deque", () => {
        var d = std::deque<int>()
        d.push(1)
        d.push(2)
        d.push(3)
        d.push(4)
        d.push(5)
        d.push(6)

        var number = 0
        for(var value in d) {
            number = (number * 10) + value
        }
        return number == 123456
    })

    test("can iterate over deque in reverse", () => {
        var d = std::deque<int>()
        d.push(1)
        d.push(2)
        d.push(3)
        d.push(4)
        d.push(5)
        d.push(6)

        var number = 0
        for(var value in d reversed) {
            number = (number * 10) + value
        }
        return number == 654321
    })

    test("continue works in deque for-in", () => {
        var d = std::deque<int>()
        d.push(1)
        d.push(2)
        d.push(3)
        d.push(4)
        var total = 0
        for(var value in d) {
            if(value == 2) continue;
            total += value
        }
        return total == 8
    })

    test("continue works in reversed deque for-in", () => {
        var d = std::deque<int>()
        d.push(1)
        d.push(2)
        d.push(3)
        d.push(4)
        var total = 0
        for(var value in d reversed) {
            if(value == 3) continue;
            total += value
        }
        return total == 7
    })
}
