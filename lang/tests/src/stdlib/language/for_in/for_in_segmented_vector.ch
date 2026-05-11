func test_for_in_segmented_vector() {
    test("can iterate over segmented_vector", () => {
        var s = std::segmented_vector<int>()
        s.push(1)
        s.push(2)
        s.push(3)
        s.push(4)
        s.push(5)
        s.push(6)
        s.push(7)

        var number = 0
        for(var value in s) {
            number = (number * 10) + value
        }

        return number == 1234567
    })

    test("can iterate over segmented_vector in reverse", () => {
        var s = std::segmented_vector<int>()
        s.push(1)
        s.push(2)
        s.push(3)
        s.push(4)
        s.push(5)
        s.push(6)
        s.push(7)

        var number = 0
        for(var value in s reversed) {
            number = (number * 10) + value
        }

        return number == 7654321
    })

    test("can iterate over segmented_vector with index", () => {
        var s = std::segmented_vector<int>()
        s.push(10)
        s.push(20)
        s.push(30)
        s.push(40)
        s.push(50)

        var total = 0
        for(var value, i in s) {
            total += value * (i + 1)
        }

        return total == 550
    })

    test("can iterate over segmented_vector with index in reverse", () => {
        var s = std::segmented_vector<int>()
        s.push(10)
        s.push(20)
        s.push(30)
        s.push(40)
        s.push(50)

        var total = 0
        for(var value, i in s reversed) {
            total += value * (i + 1)
        }

        return total == 550
    })

    test("can iterate over segmented_vector using a reference", () => {
        var s = std::segmented_vector<int>()
        s.push(2)
        s.push(3)
        s.push(5)
        s.push(7)
        s.push(11)

        var total = 0
        for(var& value in s) {
            total += *value
        }

        return total == 28
    })
}
