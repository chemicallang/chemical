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

public enum extern_enum_fruits {
    apple,
    banana,
    mango
}

public func extern_enum_get_banana() : extern_enum_fruits {
    return extern_enum_fruits.banana;
}

public func extern_enum_get_mango() : extern_enum_fruits {
    return extern_enum_fruits.mango;
}

public var extern_globe_var = 768

public func extern_globe_var_incr() {
    extern_globe_var++
}