import "@std/unordered_map.ch"
import "@std/string_view.ch"

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
        var fruit = std::string_view();
        var veg = std::string_view();
        var nut = std::string_view()
        map.find(std::string_view("fruit"), fruit)
        map.find(std::string_view("veg"), veg)
        map.find(std::string_view("nut"), nut)
        return map.size() == 3 && fruit.equals("apple") && veg.equals("kale") && nut.equals("walnut")
    })
}