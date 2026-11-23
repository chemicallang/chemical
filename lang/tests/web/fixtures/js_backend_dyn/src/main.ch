// JS backend — interface dispatch (`dyn`).
//
// `dyn I` is a fat pointer `{ first: self, second: vtable }`, and the call has to
// go through the vtable rather than through a statically known function. The
// native and JS backends must print identical output
// (`scripts/js-backend-test.sh js_backend_dyn`).

public interface Shape {
    func area(&self) : int
    func name(&self) : *char
}

public struct Square {
    var side : int
}

impl Shape for Square {
    func area(&self) : int {
        return self.side * self.side
    }

    func name(&self) : *char {
        return "square"
    }
}

public struct Rect {
    var w : int
    var h : int
}

impl Shape for Rect {
    func area(&self) : int {
        return self.w * self.h
    }

    func name(&self) : *char {
        return "rect"
    }
}

func describe(s : dyn Shape) : int {
    unsafe { printf("%s=%d\n", s.name(), s.area()) }
    return s.area()
}

public func main() : int {
    var sq = Square { side : 5 }
    var rc = Rect { w : 3, h : 4 }
    var total = 0
    total = total + describe(dyn<Shape>(sq))
    total = total + describe(dyn<Shape>(rc))
    unsafe { printf("total=%d\n", total) }
    return 0
}
