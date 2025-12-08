// usage before implementation and implementation before interface
func <T : UBI_IBI_Interface> ubi_ibi_multiply_it(value : &T) : int {
    return value.multiply()
}

// usage before implementation and implementation before interface
func ubi_ibi_multiply_it2(value : dyn UBI_IBI_Interface) : int {
    return value.multiply()
}

// usage before implementation and implementation before interface
impl UBI_IBI_Interface for UBI_IBI_Point {
    func multiply(&self) : int {
        return a * b;
    }
}

// usage before implementation and implementation before interface
struct UBI_IBI_Point {
    var a : int
    var b : int
}

// usage before implementation and implementation before interface
interface UBI_IBI_Interface {
    func multiply(&self) : int
}

func test_basic_interfaces() {

    test("usage can come before impl, impl can come before normal interface - 1", () => {
        var point = UBI_IBI_Point { a : 8, b : 6 }
        return point.multiply() == 48
    })

    test("usage can come before impl, impl can come before normal interface - 2", () => {
        var point = UBI_IBI_Point { a : 8, b : 7 }
        return ubi_ibi_multiply_it(point) == 56
    })

    test("usage can come before impl, impl can come before normal interface - 3", () => {
        var point = UBI_IBI_Point { a : 8, b : 9 }
        return ubi_ibi_multiply_it2(dyn<UBI_IBI_Interface>(point)) == 72
    })


}