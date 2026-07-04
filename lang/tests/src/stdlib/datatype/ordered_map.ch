
using namespace std;

func test_ordered_map() {
    test("ordered_map works with integer keys - 1", () => {
        var map = std::ordered_map<int, int>()
        map.insert(20, 200)
        map.insert(30, 300)
        map.insert(40, 400)
        map.insert(50, 500)
        var first : int = 0
        var second : int = 0
        var third : int = 0
        var fourth : int = 0
        map.find(20, &mut first)
        map.find(30, &mut second)
        map.find(40, &mut third)
        map.find(50, &mut fourth)
        return !map.contains(10) && first == 200 && second == 300 && third == 400 && fourth == 500
    })
    test("ordered_map works with integer keys - 2", () => {
        var map = std::ordered_map<int, int>()
        map.insert(10, 200)
        map.insert(11, 300)
        map.insert(12, 400)
        map.insert(13, 500)
        map.insert(14, 600)
        map.insert(15, 700)
        map.insert(16, 800)
        map.insert(17, 900)
        map.insert(18, 100)
        map.insert(19, 510)
        map.insert(20, 520)
        map.insert(21, 530)
        map.insert(22, 540)
        map.insert(23, 550)
        map.insert(24, 560)
        map.insert(25, 570)
        map.insert(26, 580)
        map.insert(27, 590)
        map.insert(28, 591)
        var first : int = 0
        var second : int = 0
        var third : int = 0
        var fourth : int = 0
        map.find(21, &mut first)
        map.find(27, &mut second)
        map.find(14, &mut third)
        map.find(11, &mut fourth)
        return first == 530 && second == 590 && third == 600 && fourth == 300
    })

    test("ordered_map preserves insertion order", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        var count = 0
        for(var entry in map) {
            if(count == 0 && entry.key != 1) return false
            if(count == 1 && entry.key != 2) return false
            if(count == 2 && entry.key != 3) return false
            if(count == 0 && entry.value != 100) return false
            if(count == 1 && entry.value != 200) return false
            if(count == 2 && entry.value != 300) return false
            count += 1
        }
        return count == 3
    })

    test("ordered_map update does not change order", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        map.insert(2, 250)
        var count = 0
        for(var entry in map) {
            if(count == 0 && entry.key != 1) return false
            if(count == 1 && entry.key != 2) return false
            if(count == 2 && entry.key != 3) return false
            count += 1
        }
        return count == 3
    })

    test("ordered_map erase preserves order of remaining elements", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        map.insert(4, 400)
        map.erase(2)
        var count = 0
        for(var entry in map) {
            if(count == 0 && entry.key != 1) return false
            if(count == 1 && entry.key != 3) return false
            if(count == 2 && entry.key != 4) return false
            count += 1
        }
        return count == 3
    })

    test("ordered_map erase first element", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        map.erase(1)
        var count = 0
        for(var entry in map) {
            if(count == 0 && entry.key != 2) return false
            if(count == 1 && entry.key != 3) return false
            count += 1
        }
        return count == 2
    })

    test("ordered_map erase last element", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.insert(3, 300)
        map.erase(3)
        var count = 0
        for(var entry in map) {
            if(count == 0 && entry.key != 1) return false
            if(count == 1 && entry.key != 2) return false
            count += 1
        }
        return count == 2
    })

    test("ordered_map clear", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        map.clear()
        return map.size() == 0 && map.empty() && map.isEmpty() && !map.contains(1)
    })

    test("ordered_map get_ptr", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        map.insert(2, 200)
        var ptr = map.get_ptr(1)
        if(ptr == null) return false
        return *ptr == 100
    })

    test("ordered_map get_ptr missing key", () => {
        var map = std::ordered_map<int, int>()
        map.insert(1, 100)
        return map.get_ptr(99) == null
    })

    test("ordered_map size empty isEmpty", () => {
        var map = std::ordered_map<int, int>()
        if(!map.empty()) return false
        if(!map.isEmpty()) return false
        if(map.size() != 0) return false
        map.insert(1, 100)
        if(map.empty()) return false
        if(map.isEmpty()) return false
        if(map.size() != 1) return false
        return true
    })

    test("ordered_map works with string keys", () => {
        var map = std::ordered_map<string, int>()
        map.insert(string("one"), 1)
        map.insert(string("two"), 2)
        map.insert(string("three"), 3)
        var first : int = 0
        var second : int = 0
        var third : int = 0
        map.find(string("one"), &mut first)
        map.find(string("two"), &mut second)
        map.find(string("three"), &mut third)
        return first == 1 && second == 2 && third == 3
    })
}
