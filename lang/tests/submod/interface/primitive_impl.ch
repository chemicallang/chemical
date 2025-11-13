public interface ExtPrimitiveImplInterface1 {
    func ext_give(&self) : int
}

public func <T : ExtPrimitiveImplInterface1> ext_prim_impl_interface1_give_stat_dispatch(value : T) : int {
    return value.ext_give()
}

@static
public interface PrimitiveImplInterface3 {
    func curr_mod_give2(&self) : int
}

public impl ExtPrimitiveImplInterface1 for int {
    func ext_give(&self) : int {
        return *self + 10
    }
}

public impl ExtPrimitiveImplInterface1 for long {
    func ext_give(&self) : int {
        return (*self + 20) as int
    }
}

public impl ExtPrimitiveImplInterface1 for double {
    func ext_give(&self) : int {
        return *self as int
    }
}

public impl ExtPrimitiveImplInterface1 for float {
    func ext_give(&self) : int {
        return (*self + 1.0f) as int
    }
}

public impl ExtPrimitiveImplInterface1 for bool {
    func ext_give(&self) : int {
        return if(*self) 69 else 96
    }
}

public impl ExtPrimitiveImplInterface1 for &int {
    func ext_give(&self) : int {
        return **self + 30
    }
}

public impl ExtPrimitiveImplInterface1 for &long {
    func ext_give(&self) : int {
        return (**self + 40) as int
    }
}

public impl ExtPrimitiveImplInterface1 for *int {
    func ext_give(&self) : int {
        return **self + 50
    }
}

public impl ExtPrimitiveImplInterface1 for *long {
    func ext_give(&self) : int {
        return (**self + 60) as int
    }
}

public func ext_param_call_int_prim_impl(a : int) : int {
    return a.ext_give()
}

public func ext_param_call_long_prim_impl(a : long) : int {
    return a.ext_give()
}

public func ext_param_call_ref_int_prim_impl(a : &int) : int {
    return a.ext_give()
}

public func ext_param_call_ref_long_prim_impl(a : &long) : int {
    return a.ext_give()
}

public func ext_param_call_ptr_int_prim_impl(a : *int) : int {
    return a.ext_give()
}

public func ext_param_call_ptr_long_prim_impl(a : *long) : int {
    return a.ext_give()
}