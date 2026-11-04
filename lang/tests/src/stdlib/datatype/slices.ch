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
    test("try_from_utf8 accepts valid bytes", () => {
        var result = std::try_from_utf8(std::string_view("héllo"))
        if(result is Result.Ok) {
            var Ok(value) = result else unreachable
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
    test("Box<[T]> owns its elements and supports indexing", () => {
        var values = std::vector<int>()
        values.push(5)
        values.push(6)
        var owned : Box<[int]> = values
        if(owned.size() != 2 || owned[1] != 6) { return false }
        owned[1] = 9
        return owned[1] == 9 && owned.as_span()[0] == 5
    })
    test("&mut [T] mutates through a vector view", () => {
        var values = std::vector<int>()
        values.push(1)
        values.push(2)
        var view : &mut [int] = &mut values
        view[0] = 4
        fill_mutable(&mut values)
        return view.size() == 2 && values.get(0) == 7 && values.get(1) == 7
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
    test("Box<[T]> subviews support shared and mutable views", () => {
        var values = std::vector<int>()
        values.push(5)
        values.push(6)
        var owned : Box<[int]> = values
        var shared = owned.subview(1, 2)
        var mutable = owned.subview_mut(1, 2)
        mutable[0] = 7
        return shared.size() == 1 && shared[0] == 7 && owned[1] == 7
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
    test("Box<[T]> split_at returns shared subviews", () => {
        var values = std::vector<int>()
        values.push(5)
        values.push(6)
        values.push(7)
        var owned : Box<[int]> = values
        var parts = owned.split_at(1)
        return parts.first.size() == 1 && parts.second.size() == 2 && parts.first[0] == 5 && parts.second[0] == 6
    })
    test("range syntax subviews slices", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        var middle = view[1..4]
        var prefix = view[..2]
        var suffix = view[3..]
        var all = view[..]
        return middle.size() == 3 && middle[0] == 2 && prefix.size() == 2 && suffix.size() == 2 && all.size() == 5
    })
    test("range syntax mutates a slice", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : &mut [int] = &mut arr
        var middle = view[1..4]
        middle[0] = 9
        return arr[1] == 9
    })
    test("range syntax subviews strings and boxed slices", () => {
        var text : &str = "hello"
        var word = text[1..4]
        var values = std::vector<int>()
        values.push(5)
        values.push(6)
        values.push(7)
        var owned : Box<[int]> = values
        var sub = owned[1..3]
        return word.size() == 3 && word.get(0) == 'e' && sub.size() == 2 && sub[0] == 6
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
        return again.size() == 5 && again.get(0) == 'h'
    })
    test("slice rest pattern binds the first and last elements", () => {
        var arr : [5]int = [1, 2, 3, 4, 5]
        var view : [int] = arr
        if(var [first, .., last] = view) {
            return first == 1 && last == 5
        }
        return false
    })
    test("slice rest pattern at either end", () => {
        var arr : [4]int = [10, 20, 30, 40]
        var view : [int] = arr
        var last_ok = false
        if(var [.., last] = view) {
            last_ok = last == 40
        }
        if(!last_ok) { return false }
        if(var [first, ..] = view) {
            return first == 10
        }
        return false
    })
    test("slice rest pattern requires enough elements", () => {
        var arr : [1]int = [7]
        var view : [int] = arr
        var matched = false
        if(var [a, b, ..] = view) {
            matched = true
        }
        return !matched
    })
    test("mutable slice rest pattern reads through the view", () => {
        var arr : [4]int = [10, 20, 30, 40]
        var view : &mut [int] = &mut arr
        var first_ok = false
        if(var [first, .., last] = view) {
            first_ok = first == 10 && last == 40
        }
        return first_ok
    })
    test("Box<[T]> condition pattern reads exact elements", () => {
        var values = std::vector<int>()
        values.push(1)
        values.push(2)
        values.push(3)
        var owned : Box<[int]> = values
        if(var [a, b, c] = owned) {
            return a == 1 && b == 2 && c == 3
        }
        return false
    })
    test("Box<[T]> rest pattern uses the runtime length", () => {
        var values = std::vector<int>()
        values.push(10)
        values.push(20)
        values.push(30)
        var owned : Box<[int]> = values
        if(var [first, .., last] = owned) {
            return first == 10 && last == 30
        }
        return false
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
        var owned : Box<[int]> = values
        var owned_parts = owned.split(0)
        return owned_parts.size() == 2 && owned_parts.get(1)[0] == 2
    })
}
