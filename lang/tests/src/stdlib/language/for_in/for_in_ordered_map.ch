func test_for_in_ordered_map() {
    test("can iterate over ordered_map entries", () => {
        var map = std::ordered_map<int, int>()
        map.insert(10, 100)
        map.insert(20, 200)
        map.insert(30, 300)

        var key_total = 0
        var value_total = 0
        var count = 0
        for(var entry in map) {
            key_total += entry.key
            value_total += entry.value
            count += 1
            if(count == 1) {
                if(entry.key != 10 || entry.value != 100) return false
            }
            if(count == 2) {
                if(entry.key != 20 || entry.value != 200) return false
            }
            if(count == 3) {
                if(entry.key != 30 || entry.value != 300) return false
            }
        }

        return count == 3 && key_total == 60 && value_total == 600
    })

    test("can iterate over ordered_map entries with index", () => {
        var map = std::ordered_map<int, int>()
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
            if(entry.key != i + 1) return false
        }

        return count == 4 && total == 100
    })

    test("can iterate over ordered_map entries using a reference", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 5)
        map.insert(2, 7)
        map.insert(3, 11)

        var total = 0
        for(var& entry in map) {
            total += entry.value
        }

        return total == 23
    })

    test("continue works in ordered_map for-in", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 5)
        map.insert(2, 7)
        map.insert(3, 11)
        var total = 0
        for(var entry in map) {
            if(entry.key == 2) continue;
            total += entry.value
        }
        return total == 16
    })

    test("can iterate over ordered_map in reverse", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        var count = 0
        for(var entry in map reversed) {
            count += 1
            if(count == 1 && (entry.key != 3 || entry.value != 300)) return false
            if(count == 2 && (entry.key != 2 || entry.value != 200)) return false
            if(count == 3 && (entry.key != 1 || entry.value != 100)) return false
        }
        return count == 3
    })
}
