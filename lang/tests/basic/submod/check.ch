public func extern_imported_sum(a : int, b : int) : int {
    return a + b + 20;
}

public struct extern_imported_point {

    var a : int
    var b : int

    func check_sum(&self) : int {
        return a + b;
    }

}

public var extern_globe_var = 768

public func extern_globe_var_incr() {
    extern_globe_var++
}