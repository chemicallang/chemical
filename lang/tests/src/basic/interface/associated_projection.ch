interface AssocParamProbe {
    type Item
    func assign(&mut self, value : Self::Item)
    func read(&self) : Self::Item

    // An abstract method may use the projection in parameter position; the
    // concrete implementation supplies the real type for static dispatch.
}

struct AssocParamCounter {
    var value : int
}

impl AssocParamProbe for AssocParamCounter {
    type Item = int
    func assign(&mut self, value : int) { self.value = value }
    func read(&self) : int { return self.value }
}

func <T : AssocParamProbe> assoc_probe_roundtrip(t : &mut T) {
    t.assign(t.read())
}

public func test_associated_projection_parameter() {
    test("associated type projection in parameter position dispatches statically", () => {
        var c = AssocParamCounter { value : 41 }
        c.assign(7)
        return c.read() == 7
    })
    test("generic interface call with associated parameter projection", () => {
        var c = AssocParamCounter { value : 42 }
        assoc_probe_roundtrip(&mut c)
        return c.read() == 42
    })
}

interface AssocGenericProbe {
    type Item
    func assign(&mut self, value : Self::Item)
    func read(&self) : Self::Item
}

struct AssocIntProbe {
    var value : int
}

impl AssocGenericProbe for AssocIntProbe {
    type Item = int
    func assign(&mut self, value : int) { self.value = value }
    func read(&self) : int { return self.value }
}

struct AssocLongProbe {
    var value : long
}

impl AssocGenericProbe for AssocLongProbe {
    type Item = long
    func assign(&mut self, value : long) { self.value = value }
    func read(&self) : long { return self.value }
}

struct AssocHolder<T : AssocGenericProbe> {
    var value : T::Item
}

func <T : AssocGenericProbe> assoc_generic_roundtrip(t : &mut T, value : T::Item) : T::Item {
    t.assign(value)
    return t.read()
}

func <T> assoc_where_roundtrip(t : &mut T, value : T::Item) : T::Item where T : AssocGenericProbe {
    t.assign(value)
    return t.read()
}

func <T : AssocGenericProbe> assoc_make_holder(value : T::Item) : AssocHolder<T> {
    return AssocHolder<T> { value : value }
}

func <T : AssocGenericProbe> assoc_make_vec(value : T::Item) : std::vector<T::Item> {
    var out = std::vector<T::Item>()
    out.push(value)
    return out
}

interface AssocGenericChild : AssocGenericProbe {
    func tag(&self) : int
}

struct AssocChildProbe {
    var value : int
}

impl AssocGenericProbe for AssocChildProbe {
    type Item = int
    func assign(&mut self, value : int) { self.value = value }
    func read(&self) : int { return self.value }
}

impl AssocGenericChild for AssocChildProbe {
    func tag(&self) : int { return 1 }
}

func <T : AssocGenericChild> assoc_inherited_read(t : &T) : T::Item {
    return t.read()
}

interface AssocReadOnly {
    type Item
    func read(&self) : Self::Item
}

impl AssocReadOnly for AssocIntProbe {
    type Item = int
    func read(&self) : int { return self.value }
}

struct AssocWrapper<T> {
    var inner : T
}

struct AssocEqualityHolder<T : AssocGenericProbe<Item = int>> {
    var inner : T
}

func <T : AssocGenericProbe<Item = int>> assoc_equality_roundtrip(t : &mut T, value : T::Item) : T::Item {
    t.assign(value)
    return t.read()
}

func <T> assoc_equality_where(t : &mut T, value : T::Item) : T::Item where T : AssocGenericProbe<Item = int> {
    t.assign(value)
    return t.read()
}

func <T : AssocGenericProbe<Item = int>> assoc_equality_holder(value : T) : AssocEqualityHolder<T> {
    return AssocEqualityHolder<T> { inner : value }
}

func <T : AssocGenericChild<Item = int>> assoc_equality_inherited(t : &T) : T::Item {
    return t.read()
}

impl<T : AssocReadOnly> AssocReadOnly for AssocWrapper<T> {
    type Item = T::Item
    func read(&self) : T::Item { return self.inner.read() }
}

public func test_associated_projection_generic_bounds() {
    test("generic associated projection through a bound", () => {
        var i = AssocIntProbe { value : 0 }
        var l = AssocLongProbe { value : 0 }
        var a = assoc_generic_roundtrip(&mut i, 11)
        var b = assoc_generic_roundtrip(&mut l, 12 as long)
        if(a != 11 || b != 12) return false
        var w = assoc_where_roundtrip(&mut i, 13)
        if(w != 13) return false
        var h = assoc_make_holder<AssocIntProbe>(17)
        if(h.value != 17) return false
        var v = assoc_make_vec<AssocIntProbe>(19)
        if(v.size() != 1 || v.get(0) != 19) return false
        var c = AssocChildProbe { value : 23 }
        if(assoc_inherited_read(&c) != 23) return false
        var wrap = AssocWrapper<AssocIntProbe> { inner : AssocIntProbe { value : 29 } }
        if(wrap.read() != 29) return false
        var eq = AssocIntProbe { value : 0 }
        if(assoc_equality_roundtrip(&mut eq, 31) != 31) return false
        if(assoc_equality_where(&mut eq, 37) != 37) return false
        var holder = assoc_equality_holder(AssocIntProbe { value : 41 })
        if(holder.inner.read() != 41) return false
        var child = AssocChildProbe { value : 43 }
        return assoc_equality_inherited(&child) == 43
    })
}
