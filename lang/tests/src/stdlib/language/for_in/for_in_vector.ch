func test_for_in_vector() {
    test("can iterate over vector", () => {
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i in s) {
            total += i
        }
        return total == 60
    })
    test("can iterate over vector in reverse", () => {
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i in s reversed) {
            total += i
        }
        return total == 60
    })
    test("can iterate over vector using a reference", () => {
        var s = std::vector<int>()
        s.push(20);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var& i in s) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over vector using a reference in reverse", () => {
        var s = std::vector<int>()
        s.push(20);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var& i in s reversed) {
            total += *i
        }
        return total == 70
    })
}