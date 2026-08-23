using std::string;
using std::string_view;

func string_eq(a : *string, b : string_view) : bool {
    if(a.size() != b.size()) { return false }
    var ad = a.data()
    var bd = b.data()
    var i = 0
    while(i < a.size()) {
        if(ad[i] != bd[i]) { return false }
        i += 1
    }
    return true
}

public func main(argc : int, argv : **char) : int {
    return test_runner(argc, argv)
}
