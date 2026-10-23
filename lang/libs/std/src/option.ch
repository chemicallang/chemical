public namespace std {

public variant Option<T> {
    Some(value : T)
    None()

    func take(&self) : T {
        if(self is Option.None) {
            panic("cannot take on a option that's none")
        }
        var Some(value) = self else unreachable
        var temp : T
        unsafe {
            memcpy(&raw mut temp, &raw value, sizeof(T))
            // initialize without destruction
            new(&raw self) Option.None<T>()
            return temp;
        }
    }

}

impl<T : core::ops::Clone> core::ops::Clone for Option<T> {
    public func clone(&self) : Option<T> {
        if(self is Option.Some) {
            var Some(value) = self else unreachable
            var typed = value as T
            var out = typed.clone()
            intrinsics::forget(typed)
            return Option.Some<T>(out)
        }
        return Option.None<T>()
    }
}
}