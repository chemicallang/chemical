using namespace std;

func slice_sum(values : &[int]) : int {
    var total = 0
    for(var i = 0u; i < values.size(); i++) {
        total += values[i]
    }
    return total
}

func first_str_byte(value : &str) : char {
    return value.get(0)
}

func test_slices() {
    test("&[T] views an array and supports indexing", () => {
        var arr : [4]int = [1, 2, 3, 4]
        var values : &[int] = arr
        return values.size() == 4 && values[2] == 3 && values[0] == 1
    })
    test("&[T] passes through function parameters", () => {
        var arr : [4]int = [1, 2, 3, 4]
        return slice_sum(arr) == 10
    })
    test("&[T] views a vector", () => {
        var values = std::vector<int>()
        values.push(5)
        values.push(6)
        var view : &[int] = &values
        return view.size() == 2 && view[1] == 6
    })
    test("&str views a UTF-8 literal", () => {
        var text : &str = "hello"
        return text.size() == 5 && text.get(0) == 'h' && !text.empty()
    })
    test("&str passes through function parameters", () => {
        return first_str_byte("A") == 'A'
    })
    test("&str views a string and validates at the boundary", () => {
        var value = std::string("hello")
        var text : &str = &value
        return text.size() == 5 && text.get(4) == 'o'
    })
    test("&str can be copied without owning the bytes", () => {
        var text : &str = "hello"
        var copy = text
        return copy.size() == 5 && copy.to_view().size() == 5
    })
    test("&str accepts multibyte UTF-8", () => {
        var text : &str = "héllo 😀"
        return text.size() == 11
    })
}
