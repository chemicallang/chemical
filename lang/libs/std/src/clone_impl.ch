// Copyright (c) Chemical Language Foundation 2026.

using core::ops::Clone;

public namespace std {

impl Clone for int {
    public func clone(&self) : int {
        return *self
    }
}

impl Clone for uint {
    public func clone(&self) : uint {
        return *self
    }
}

impl Clone for i8 {
    public func clone(&self) : i8 {
        return *self
    }
}

impl Clone for i16 {
    public func clone(&self) : i16 {
        return *self
    }
}

impl Clone for i32 {
    public func clone(&self) : i32 {
        return *self
    }
}

impl Clone for i64 {
    public func clone(&self) : i64 {
        return *self
    }
}

impl Clone for u8 {
    public func clone(&self) : u8 {
        return *self
    }
}

impl Clone for u16 {
    public func clone(&self) : u16 {
        return *self
    }
}

impl Clone for u32 {
    public func clone(&self) : u32 {
        return *self
    }
}

impl Clone for u64 {
    public func clone(&self) : u64 {
        return *self
    }
}

impl Clone for bool {
    public func clone(&self) : bool {
        return *self
    }
}

impl Clone for char {
    public func clone(&self) : char {
        return *self
    }
}

impl Clone for float {
    public func clone(&self) : float {
        return *self
    }
}

impl Clone for double {
    public func clone(&self) : double {
        return *self
    }
}

impl Clone for string_view {
    public func clone(&self) : string_view {
        return *self
    }
}

impl<T> Clone for span<T> {
    public func clone(&self) : span<T> {
        return *self
    }
}

}
