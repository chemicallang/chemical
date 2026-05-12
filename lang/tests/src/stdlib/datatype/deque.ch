func test_deque() {
    test("deque stores and retrieves values across chunks", () => {
        var d = std::deque<int>()
        d.push(10)
        d.push(20)
        d.push(30)
        d.push(40)
        d.push(50)
        d.push(60)

        var first = d.get_ptr(0)
        var fourth = d.get_ptr(3)
        var fifth = d.get_ptr(4)
        var sixth = d.get_ptr(5)

        if(first == null || fourth == null || fifth == null || sixth == null) {
            return false
        }

        return d.size() == 6 && *first == 10 && *fourth == 40 && *fifth == 50 && *sixth == 60
    })

    test("deque reports empty and size", () => {
        var d = std::deque<int>()
        if(!d.empty() || d.size() != 0) return false
        d.push(1)
        d.push(2)
        return !d.empty() && d.size() == 2
    })

    test("deque clear resets state", () => {
        var d = std::deque<int>()
        d.push(1)
        d.push(2)
        d.push(3)
        d.clear()
        return d.empty() && d.size() == 0
    })
}
