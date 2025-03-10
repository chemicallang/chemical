import "@std/unordered_map.ch"
import "@std/string_view.ch"
import "@std/string.ch"

using namespace std;

func test_unordered_map() {
    test("unordered_map works with integer keys - 1", () => {
        var map = std::unordered_map<int, int>()
        map.insert(20, 200)
        map.insert(30, 300)
        map.insert(40, 400)
        map.insert(50, 500)
        var first : int = 0
        var second : int = 0
        var third : int = 0
        var fourth : int = 0
        map.find(20, first)
        map.find(30, second)
        map.find(40, third)
        map.find(50, fourth)
        return !map.contains(10) && first == 200 && second == 300 && third == 400 && fourth == 500
    })
    test("unordered_map works with integer keys - 2", () => {
        var map = std::unordered_map<int, int>()
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
        map.find(21, first)
        map.find(27, second)
        map.find(14, third)
        map.find(11, fourth)
        return first == 530 && second == 590 && third == 600 && fourth == 300
    })
    test("unordered map works with string views as keys and values", () => {
        var map = std::unordered_map<std::string_view, std::string_view>()
        map.insert(std::string_view("fruit"), std::string_view("apple"))
        map.insert(std::string_view("veg"), std::string_view("kale"))
        map.insert(std::string_view("nut"), std::string_view("walnut"))
        var fruit = map.get_ptr(std::string_view("fruit"))
        var veg = map.get_ptr(std::string_view("veg"))
        var nut = map.get_ptr(std::string_view("nut"))
        return map.size() == 3 && fruit != null && veg != null && nut != null && fruit.equals("apple") && veg.equals("kale") && nut.equals("walnut")
    })
    test("unordered map works with string views as keys and values when resized", () => {
        var map = std::unordered_map<std::string_view, std::string_view>()
        map.insert(std::string_view("fruit"), std::string_view("apple"))
        map.insert(std::string_view("veg"), std::string_view("kale"))
        map.insert(std::string_view("nut"), std::string_view("walnut"))
        map.insert(std::string_view("10"), std::string_view("30"))
        map.insert(std::string_view("11"), std::string_view("31"))
        map.insert(std::string_view("12"), std::string_view("32"))
        map.insert(std::string_view("13"), std::string_view("33"))
        map.insert(std::string_view("14"), std::string_view("34"))
        map.insert(std::string_view("15"), std::string_view("35"))
        map.insert(std::string_view("16"), std::string_view("36"))
        map.insert(std::string_view("17"), std::string_view("37"))
        map.insert(std::string_view("18"), std::string_view("38"))
        map.insert(std::string_view("19"), std::string_view("39"))
        map.insert(std::string_view("20"), std::string_view("40"))
        map.insert(std::string_view("21"), std::string_view("41"))
        map.insert(std::string_view("22"), std::string_view("42"))
        map.insert(std::string_view("23"), std::string_view("43"))
        map.insert(std::string_view("24"), std::string_view("44"))
        map.insert(std::string_view("25"), std::string_view("45"))
        map.insert(std::string_view("26"), std::string_view("46"))
        var fruit = map.get_ptr(std::string_view("fruit"))
        var veg = map.get_ptr(std::string_view("veg"))
        var nut = map.get_ptr(std::string_view("nut"))
        var rand1 = map.get_ptr(std::string_view("23"))
        var rand2 = map.get_ptr(std::string_view("19"))
        var rand3 = map.get_ptr(std::string_view("26"))
        if(fruit == null || veg == null || nut == null || rand1 == null || rand2 == null || rand3 == null) {
            printf("null returned !!\n");
            return false;
        }
        return fruit.equals("apple") && veg.equals("kale") && nut.equals("walnut") && rand1.equals("43") && rand2.equals("39") && rand3.equals("46")
    })
    test("unordered map works with string as keys and values", () => {
        var map = std::unordered_map<std::string, std::string>()
        map.insert(std::string("fruit"), std::string("apple"))
        map.insert(std::string("veg"), std::string("kale"))
        map.insert(std::string("nut"), std::string("walnut"))
        var fruit = map.get_ptr(std::string("fruit"))
        var veg = map.get_ptr(std::string("veg"))
        var nut = map.get_ptr(std::string("nut"))
        if(fruit == null || veg == null || nut == null) {
            printf("null returned !!\n");
            return false;
        }
        return map.size() == 3 && fruit.equals(std::string("apple")) && veg.equals(std::string("kale")) && nut.equals(std::string("walnut"))
    })
    test("unordered map works with strings as keys and values when resized", () => {
        var map = std::unordered_map<std::string, std::string>()
        map.insert(std::string("fruit"), std::string("apple"))
        map.insert(std::string("veg"), std::string("kale"))
        map.insert(std::string("nut"), std::string("walnut"))
        map.insert(std::string("10"), std::string("30"))
        map.insert(std::string("11"), std::string("31"))
        map.insert(std::string("12"), std::string("32"))
        map.insert(std::string("13"), std::string("33"))
        map.insert(std::string("14"), std::string("34"))
        map.insert(std::string("15"), std::string("35"))
        map.insert(std::string("16"), std::string("36"))
        map.insert(std::string("17"), std::string("37"))
        map.insert(std::string("18"), std::string("38"))
        map.insert(std::string("19"), std::string("39"))
        map.insert(std::string("20"), std::string("40"))
        map.insert(std::string("21"), std::string("41"))
        map.insert(std::string("22"), std::string("42"))
        map.insert(std::string("23"), std::string("43"))
        map.insert(std::string("24"), std::string("44"))
        map.insert(std::string("25"), std::string("45"))
        map.insert(std::string("26"), std::string("46"))

        var fruit = map.get_ptr(std::string("fruit"));
        var veg = map.get_ptr(std::string("veg"))
        var nut = map.get_ptr(std::string("nut"))
        var rand1 = map.get_ptr(std::string("23"))
        var rand2 = map.get_ptr(std::string("19"))
        var rand3 = map.get_ptr(std::string("26"))

        if(fruit == null || veg == null || nut == null || rand1 == null || rand2 == null || rand3 == null) {
            printf("null returned !!\n");
            return false;
        }

        return fruit.equals("apple") && veg.equals("kale") && nut.equals("walnut") && rand1.equals("43") && rand2.equals("39") && rand3.equals("46")
    })
}