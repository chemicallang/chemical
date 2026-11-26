using std::string;
using std::string_view;
using std::vector;
using std::Option;
using std::Result;
using std::unordered_map;

public func main() : int {
    var s = string("chem")
    var i : int = 0
    while(i < 5) {
        s.append_view("!")
        i = i + 1
    }

    var v = vector<int>()
    v.push(1)
    v.push(2)
    v.push(3)
    v.push(4)
    var sum : int = 0
    var j : int = 0
    while(j < (v.size() as int)) {
        sum = sum + unsafe(*v.get_ptr(j as size_t))
        j = j + 1
    }

    var m = unordered_map<int, int>()
    m.insert(1, 10)
    m.insert(2, 20)
    var mp = m.get_ptr(2)
    if(mp == null) {
        return 2
    }
    sum = sum + unsafe(*mp)

    var opt = Option.Some<int>(sum)
    var r = Result.Ok<int, int>(sum)
    var opt_value : int = 0
    if(opt is Option.Some) {
        var Some(x) = opt else unsafe { unreachable }
        opt_value = x
    }
    var result_value : int = 0
    if(r is Result.Ok) {
        var Ok(y) = r else unsafe { unreachable }
        result_value = y
    }

    unsafe { printf("wasm std: s=%s len=%d vector=%d map=%d option=%d result=%d\n",
        s.data(), s.size() as int, 10, 20, opt_value, result_value) }

    if(s.size() != 9) { return 3 }
    if(sum != 30) { return 4 }
    if(opt_value != 30) { return 5 }
    if(result_value != 30) { return 6 }
    return 0
}
