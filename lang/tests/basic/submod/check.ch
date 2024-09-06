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

public variant extern_imported_opt {

    None()
    Some(value : int)

}

public func get_extern_imported_opt_none() : extern_imported_opt {
    return extern_imported_opt.None
}

public func get_extern_imported_opt_some() : extern_imported_opt {
    return extern_imported_opt.Some(50)
}

public func get_extern_imported_opt_value(x : extern_imported_opt)  : int {
    switch(x) {
        extern_imported_opt.None() => {
            return -1
        }
        extern_imported_opt.Some(value) => {
            return value;
        }
    }
}

public struct extern_unused_gen_struct<T> {

    var a : T
    var b : T

    func unused_sum(&self) : T {
        return a + b;
    }

}

public struct extern_unused_gen_struct2<T> {

    var a : T
    var b : T

    func unused_sum(&self) : T {
        return a + b + 4;
    }

}

public struct extern_used_gen_struct<T> {

    var a : T
    var b : T

    func used_sum(&self) : T {
        return a + b + 3;
    }

}

public func extern_gen_struct_user(str : extern_used_gen_struct<int>) : int {
    return str.a + str.b;
}