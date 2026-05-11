func test_for_in_unordered_map() {
    test("can iterate over unordered_map entries", () => {
        var map = std::unordered_map<int, int>()
        map.insert(10, 100)
        map.insert(20, 200)
        map.insert(30, 300)

        var key_total = 0
        var value_total = 0
        for(var entry in map) {
            key_total += entry.key
            value_total += entry.value
        }

        return key_total == 60 && value_total == 600
    })

    test("can iterate over unordered_map entries with index", () => {
        var map = std::unordered_map<int, int>()
        map.insert(1, 10)
        map.insert(2, 20)
        map.insert(3, 30)
        map.insert(4, 40)

        var count = 0
        var total = 0
        for(var entry, i in map) {
            count += 1
            total += entry.value
            if(i >= 4) return false
        }

        return count == 4 && total == 100
    })

    test("can iterate over unordered_map entries using a reference", () => {
        var map = std::unordered_map<int, int>()
        map.insert(1, 5)
        map.insert(2, 7)
        map.insert(3, 11)

        var total = 0
        for(var& entry in map) {
            total += entry.value
        }

        return total == 23
    })
}
