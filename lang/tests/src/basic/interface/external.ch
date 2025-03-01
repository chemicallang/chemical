import "@submod/interface/external.ch"

public struct ExtPubNormInterfaceImpl3 : ExtPubNormInterface {

    var a : int
    var b : int
    var c : int
    var d : int

    @override
    func sum(&self) : int {
        return a + b + c + d;
    }

}

func dyn_sum_pub_norm_inter(thing : dyn ExtPubNormInterface) : int {
    return thing.sum()
}

public func test_ext_pub_norm_inter_impl1_again() : int {
    var ext = ExtPubNormInterfaceImpl1 { a : 10, b : 20 }
    return ext.sum();
}

public func test_ext_pub_norm_inter_impl2_again() : int {
    var ext = ExtPubNormInterfaceImpl2 { a : 10, b : 20, c : 30 }
    return ext.sum();
}

public func test_ext_pub_norm_inter_impl3() : int {
    var ext = ExtPubNormInterfaceImpl3 { a : 10, b : 20, c : 30, d : 40 }
    return ext.sum();
}

public func test_external_interfaces() {

    test("external public normal interfaces work through direct structs - 1", () => {
        return test_ext_pub_norm_inter_impl1() == 30
    })

    test("external public normal interfaces work through direct structs - 2", () => {
        return test_ext_pub_norm_inter_impl2() == 60
    })

    test("external public normal interfaces work in current mod through direct structs - 3", () => {
        return test_ext_pub_norm_inter_impl1_again() == 30
    })

    test("external public normal interfaces work in current mod through direct structs - 4", () => {
        return test_ext_pub_norm_inter_impl2_again() == 60
    })

    test("external public normal interfaces implemented in current mod work through direct structs", () => {
        return test_ext_pub_norm_inter_impl3() == 100
    })

    test("external public normal interface works in current mod through dynamic call - 1", () => {
        var ext = ExtPubNormInterfaceImpl1 { a : 10, b : 20 }
        return dyn_sum_pub_norm_inter(ext) == 30
    })

    test("external public normal interface works in current mod through dynamic call - 2", () => {
        var ext = ExtPubNormInterfaceImpl2 { a : 10, b : 20, c : 30 }
        return dyn_sum_pub_norm_inter(ext) == 60
    })

    test("external public normal interface implemented in current mod works through dynamic call", () => {
        var ext = ExtPubNormInterfaceImpl3 { a : 10, b : 20, c : 30, d : 40 }
        return dyn_sum_pub_norm_inter(ext) == 100
    })

}