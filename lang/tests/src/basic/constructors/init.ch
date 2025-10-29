struct InitDefConsStruct {

    var a : int
    var b : int

    @make
    func make() {
        init {
            a = 98
            b = 87
        }
    }

}

struct InitBlockAutoDefConsCall {

    var d : InitDefConsStruct

}

struct InitBlockManualDefConsCall {

    var d : InitDefConsStruct

    @make
    func make() {
        init {
            d = InitDefConsStruct()
        }
    }

}

struct InitBlockManualDefConsCall2 {

    var d : InitDefConsStruct

    @make
    func make() {
        init {
            d(InitDefConsStruct())
        }
    }

}

struct InitBlockManualDefConsCall3 : InitDefConsStruct {
}

struct InitBlockManualDefConsCall4 : InitDefConsStruct {

    @make
    func make() {

    }

}

struct InitBlockManualDefConsCall5 : InitDefConsStruct {

    @make
    func make() {
        init {}
    }

}

struct InitBlockManualDefConsCall6 : InitDefConsStruct {

    @make
    func make() {
        init {
            InitDefConsStruct = InitDefConsStruct()
        }
    }

}

struct InitBlockManualDefConsCall7 : InitDefConsStruct {

    @make
    func make() {
        init {
            InitDefConsStruct(InitDefConsStruct())
        }
    }

}

func test_constructors_with_init() {

    test("default auto constructor call in init block works", () => {
        var c = InitBlockAutoDefConsCall()
        return c.d.a == 98 && c.d.b == 87
    })

    test("default manual constructor call in init block works - 1", () => {
        var c = InitBlockManualDefConsCall()
        return c.d.a == 98 && c.d.b == 87
    })

    test("default manual constructor call in init block works - 2", () => {
        var c = InitBlockManualDefConsCall2()
        return c.d.a == 98 && c.d.b == 87
    })

    test("default manual constructor call in init block works - 3", () => {
        var c = InitBlockManualDefConsCall3()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 4", () => {
        var c = InitBlockManualDefConsCall4()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 5", () => {
        var c = InitBlockManualDefConsCall5()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 6", () => {
        var c = InitBlockManualDefConsCall6()
        return c.a == 98 && c.b == 87
    })
    test("default manual constructor call in init block works - 7", () => {
        var c = InitBlockManualDefConsCall7()
        return c.a == 98 && c.b == 87
    })

}