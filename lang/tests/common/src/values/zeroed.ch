struct ZeroInitializable {
    var a : int
    var b : int
}

func give_me_zi(zi : ZeroInitializable) : int {
        unsafe {
        return zi.a + zi.b

        }
    }

func give_me_zi_ptr(zi : *ZeroInitializable) : int {
        unsafe {
        return zi.a + zi.b

        }
    }

struct ConstructibleZeroInitializable {
    var a : int
    var b : int
    @make
    func make() {
        unsafe {     return { a : 10, b : 10 } 
        }
    }
}

struct ConstructibleZeroInitializable2 {
    var a : int
    var b : int
    @make
    func make() {
        unsafe {     return { a : 10, b : 10 } 
        }
    }
}
unsafe impl Zeroable for ConstructibleZeroInitializable2 { }

struct DestructibleZeroInitializable {
    var a : int
    var b : int
    @delete
    func delete(&mut self) { }
}

struct DestructibleZeroInitializable2 {
    var a : int
    var b : int
    @delete
    func delete(&mut self) { }
}
unsafe impl Zeroable for DestructibleZeroInitializable2 { }

func <T> gen_zeroed() : T where T: Zeroable {
        unsafe {
        return zeroed<T>();

        }
    }

func test_zeroed_value() {
        unsafe {
        test("zero initializable works", () => {
            var z = zeroed<ZeroInitializable>()
            return z.a == 0 && z.b == 0
        })
        test("zero initializable works when sending to function calls", () => {
            return give_me_zi(zeroed<ZeroInitializable>()) == 0
        })
        test("zero initializable works when taking address", () => {
            return give_me_zi_ptr(&raw zeroed<ZeroInitializable>()) == 0
        })
        test("zero initializable works inside generic containers - 1", () => {
            var z = gen_zeroed<ZeroInitializable>()
            return z.a == 0 && z.b == 0
        })
        test("zero initializable works inside generic containers - 2", () => {
            return give_me_zi(gen_zeroed<ZeroInitializable>()) == 0
        })
        test("constructable structs work with unsafe zeroed", () => {
            var x = zeroed:unsafe<ConstructibleZeroInitializable>()
            return x.a == 0 && x.b == 0
        })
        test("constructable structs work with unsafe impl Zeroable", () => {
            var x = zeroed<ConstructibleZeroInitializable2>()
            return x.a == 0 && x.b == 0
        })
        test("destructible structs work with unsafe zeroed", () => {
            var x = zeroed:unsafe<DestructibleZeroInitializable>()
            return x.a == 0 && x.b == 0
        })
        test("destructible structs work with unsafe impl Zeroable", () => {
            var x = zeroed<DestructibleZeroInitializable2>()
            return x.a == 0 && x.b == 0
        })

        }
    }