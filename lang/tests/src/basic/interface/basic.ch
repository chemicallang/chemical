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

func get_multiply_8334(value : dyn IndirectlyInheritedInterface9328) : int {
    return value.multiply()
}

interface IndirectlyInheritedInterface9328 {
    func multiply(&self) : int
}

interface DelegateInterface2342 : IndirectlyInheritedInterface9328 {
}

struct Point3828 : DelegateInterface2342 {
    var a : int
    var b : int

    @override
    func multiply(&self) : int {
        return a * b;
    }
}

func use_of_static_inter_bef_def(s : &static_interface_after_usage) : int {
    return s.give()
}

struct impl_of_static_inter_bef_def : static_interface_after_usage {
    @override
    func give(&self) : int {
        return 436;
    }
}

@static
interface static_interface_after_usage {
    func give(&self) : int
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

    test("dynamic calls on indirectly inherited interfaces works", () => {
        var p = Point3828 { a : 10, b : 34 }
        return get_multiply_8334(dyn<IndirectlyInheritedInterface9328>(p)) == 340
    })

    test("use and impl of static interface before definition works", () => {
        var i = impl_of_static_inter_bef_def {};
        return i.give() == 436
    })

    test("use and impl of static interface before definition works", () => {
        var i = impl_of_static_inter_bef_def {};
        return use_of_static_inter_bef_def(i) == 436
    })

}