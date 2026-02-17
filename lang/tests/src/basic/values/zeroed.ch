struct ZeroInitializable {
    var a : int
    var b : int
}

func give_me_zi(zi : ZeroInitializable) : int {
    return zi.a + zi.b
}

func give_me_zi_ptr(zi : *ZeroInitializable) : int {
    return zi.a + zi.b
}

struct ConstructibleZeroInitializable {
    var a : int
    var b : int
    @make
    func make() { return { a : 10, b : 10 } }
}

@allow_zeroed
struct ConstructibleZeroInitializable2 {
    var a : int
    var b : int
    @make
    func make() { return { a : 10, b : 10 } }
}

struct DestructibleZeroInitializable {
    var a : int
    var b : int
    @delete
    func delete(&mut self) { }
}

@allow_zeroed
struct DestructibleZeroInitializable2 {
    var a : int
    var b : int
    @delete
    func delete(&mut self) { }
}

func test_zeroed_value() {
    test("zero initializable works", () => {
        var z = zeroed<ZeroInitializable>()
        return z.a == 0 && z.b == 0
    })
    test("zero initializable works when sending to function calls", () => {
        return give_me_zi(zeroed<ZeroInitializable>()) == 0
    })
    test("zero initializable works when taking address", () => {
        return give_me_zi_ptr(&zeroed<ZeroInitializable>()) == 0
    })
    test("constructable structs work with unsafe zeroed", () => {
        var x = zeroed:unsafe<ConstructibleZeroInitializable>()
        return x.a == 0 && x.b == 0
    })
    test("constructable structs work with allow_zeroed", () => {
        var x = zeroed<ConstructibleZeroInitializable2>()
        return x.a == 0 && x.b == 0
    })
    test("destructible structs work with unsafe zeroed", () => {
        var x = zeroed:unsafe<DestructibleZeroInitializable>()
        return x.a == 0 && x.b == 0
    })
    test("destructible structs work with allow_zeroed", () => {
        var x = zeroed<DestructibleZeroInitializable2>()
        return x.a == 0 && x.b == 0
    })
}