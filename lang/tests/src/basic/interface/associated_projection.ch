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
