using namespace std;

func slice_sum(values : &[int]) : int {
    var total = 0
    for(var i = 0u; i < values.size(); i++) {
        total += values[i]
    }
    return total
}

func first_str_byte(value : &str) : char {
    return value.at(0)
}

func bare_slice_total(values : [int]) : int {
    var total = 0
    for(var i = 0u; i < values.size(); i++) {
        total += values[i]
    }
    return total
}

func <'a> explicit_slice_identity(values : &'a [int]) : &'a [int] {
    return values
}

func <'a, 'b> explicit_slice_longer(values : &'a [int]) : &'b [int] where 'a : 'b {
    return values
}

func <'a> explicit_str_identity(value : &'a str) : &'a str {
    return value
}

func fill_mutable(values : &mut [int]) {
    for(var i = 0u; i < values.size(); i++) {
        values[i] = 7
    }
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
        return text.size() == 5 && text.at(0) == 'h' && !text.empty()
    })
    test("&str passes through function parameters", () => {
        return first_str_byte("A") == 'A'
    })
    test("&str views a string and validates at the boundary", () => {
        var value = std::string("hello")
        var text : &str = &value
        return text.size() == 5 && text.at(4) == 'o'
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
    test("try_from_utf8 accepts valid bytes", () => {
        var result = std::try_from_utf8(std::string_view("héllo"))
        if(result is Result.Ok) {
            var Ok(value) = result else unsafe { unreachable }
            return value.size() == 6
        }
        return false
    })
    test("try_from_utf8 rejects invalid bytes", () => {
        var bytes : [1]char = [0xFF as char]
        var view = unsafe(std::string_view(&raw mut bytes[0], 1))
        var result = std::try_from_utf8(view)
        return result is Result.Err
    })
    test("[T] is a bare borrowed slice view", () => {
        var arr : [4]int = [1, 2, 3, 4]
        var view : [int] = arr
        return view.size() == 4 && view[2] == 3 && bare_slice_total(arr) == 10
    })
    test("[T] views a vector", () => {
        var values = std::vector<int>()
        values.push(1)
        values.push(2)
        var view : [int] = &values
        return view.size() == 2 && view[1] == 2
    })
    test("vector.as_span views elements without owning them", () => {
        var values = std::vector<int>()
        values.push(4)
        values.push(5)
        var view = values.as_span()
        return view.size() == 2 && unsafe(*view.at(1)) == 5
    })
    test("vector.as_mut_span mutates elements", () => {
        var values = std::vector<int>()
        values.push(1)
        var view = values.as_mut_span()
        view.set(0, 9)
        return values.at(0) == 9
    })
    test("vector<T> owns its elements and supports indexing", () => {
        var owned = std::vector<int>()
        owned.push(5)
        owned.push(6)
        if(owned.size() != 2 || owned.at(1) != 6) { return false }
        owned.set(1, 9)
        var view : [int] = &owned
        return owned.at(1) == 9 && view[0] == 5
    })
    test("&mut [T] mutates through a vector view", () => {
        var values = std::vector<int>()
        values.push(1)
        values.push(2)
        var view : &mut [int] = &mut values
        view[0] = 4
        fill_mutable(&mut values)
        return view.size() == 2 && values.at(0) == 7 && values.at(1) == 7
    })
    test("&mut [T] mutates an array view", () => {
        var arr : [3]int = [1, 2, 3]
        var view : &mut [int] = &mut arr
        view[0] = 7
        return view[0] == 7 && arr[0] == 7
    })
    test("&mut [T] passes an array to a mutating function", () => {
        var arr : [3]int = [1, 2, 3]
        fill_mutable(&mut arr)
        return arr[0] == 7 && arr[1] == 7 && arr[2] == 7
    })
    test("span subview borrows a range", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var sub = view.subview(1, 4)
        return sub.size() == 3 && sub[0] == 2 && sub[2] == 4
    })
    test("mut_span subview writes through the parent", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : &mut [int] = &mut arr
        var sub = view.subview(1, 4)
        sub[0] = 9
        return arr[1] == 9
    })
    test("vector<T> subviews support shared and mutable views", () => {
        var owned = std::vector<int>()
        owned.push(5)
        owned.push(6)
        var view : &mut [int] = &mut owned
        var mutable = view.subview(1, 2)
        mutable[0] = 7
        var shared : [int] = &owned
        return shared.size() == 2 && shared[1] == 7 && owned.at(1) == 7
    })
    test("split_at returns two shared subviews", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var parts = view.split_at(2)
        return parts.first.size() == 2 && parts.second.size() == 3 && parts.first[1] == 2 && parts.second[0] == 3
    })
    test("split_at returns two mutable subviews", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : &mut [int] = &mut arr
        var parts = view.split_at(2)
        parts.first[0] = 9
        parts.second[0] = 8
        return arr[0] == 9 && arr[2] == 8
    })
    test("vector<T> split_at returns shared subviews", () => {
        var owned = std::vector<int>()
        owned.push(5)
        owned.push(6)
        owned.push(7)
        var view : [int] = &owned
        var parts = view.split_at(1)
        return parts.first.size() == 1 && parts.second.size() == 2 && parts.first[0] == 5 && parts.second[0] == 6
    })
    test("subview method slices a range", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var middle = view.subview(1, 4)
        var prefix = view.subview(0, 2)
        var suffix = view.subview(3, 5)
        var all = view.subview(0, 5)
        return middle.size() == 3 && middle[0] == 2 && prefix.size() == 2 && suffix.size() == 2 && all.size() == 5
    })
    test("subview method mutates a slice", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : &mut [int] = &mut arr
        var middle = view.subview(1, 4)
        middle[0] = 9
        return arr[1] == 9
    })
    test("subview method slices strings and vectors", () => {
        var text : &str = "hello"
        var word = text.subview(1, 4)
        var owned = std::vector<int>()
        owned.push(5)
        owned.push(6)
        owned.push(7)
        var view : [int] = &owned
        var sub = view.subview(1, 3)
        return word.size() == 3 && word.at(0) == 'e' && sub.size() == 2 && sub[0] == 6
    })
    test("windows returns overlapping subviews", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var windows = view.windows(3)
        return windows.size() == 3 && windows.get(0)[0] == 1 && windows.get(2)[0] == 3
    })
    test("chunks returns disjoint subviews", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var chunks = view.chunks(2)
        return chunks.size() == 3 && chunks.get(0).size() == 2 && chunks.get(1)[0] == 3 && chunks.get(2).size() == 1
    })
    test("split_by partitions on a predicate", () => {
        var arr : [7]int = [1, 0, 2, 3, 0, 0, 4]
        var view : [int] = arr
        var parts = view.split_by(||(x : &int) : bool => *x == 0)
        return parts.size() == 4 && parts.get(0)[0] == 1 && parts.get(1)[0] == 2 && parts.get(2).size() == 0 && parts.get(3)[0] == 4
    })
    test("explicit slice and str lifetimes", () => {
        var arr : [3]int = [7, 8, 9]
        var same = explicit_slice_identity(arr)
        if(same.size() != 3 || same[1] != 8) return false
        var longer = explicit_slice_longer(arr)
        if(longer[2] != 9) return false
        var text : &str = "hello"
        var again = explicit_str_identity(text)
        return again.size() == 5 && again.at(0) == 'h'
    })
    test("flat fixed-array rest pattern binds the first and last elements", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var [first, .., last] = arr
        return first == 1 && last == 5
    })
    test("flat fixed-array rest pattern at either end", () => {
        var arr : [4]int = [10, 20, 30, 40]
        var [.., last] = arr
        if(last != 40) { return false }
        var [first, ..] = arr
        return first == 10
    })
    test("flat fixed-array exact destructuring binds all elements", () => {
        var arr : [4]int = [10, 20, 30, 40]
        var [a, b, c, d] = arr
        return a == 10 && b == 20 && c == 30 && d == 40
    })
    test("split partitions on a delimiter", () => {
        var arr : [7]int = [1, 0, 2, 3, 0, 0, 4]
        var view : [int] = arr
        var parts = view.split(0)
        if(parts.size() != 4 || parts.get(0)[0] != 1 || parts.get(1)[0] != 2) return false
        if(parts.get(2).size() != 0 || parts.get(3)[0] != 4) return false
        var text_arr : [5]char = ['a', ',', 'b', ',', 'c']
        var text : [char] = text_arr
        var text_parts = text.split(',')
        if(text_parts.size() != 3 || text_parts.get(1)[0] != 'b') return false
        var values = std::vector<int>()
        values.push(1)
        values.push(0)
        values.push(2)
        var owned : [int] = &values
        var owned_parts = owned.split(0)
        return owned_parts.size() == 2 && owned_parts.get(1)[0] == 2
    })
}
