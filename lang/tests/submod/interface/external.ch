public interface ExtPubNormInterface {

    func sum(&self) : int

}

public struct ExtPubNormInterfaceImpl1 : ExtPubNormInterface {

    var a : int
    var b : int

    @override
    func sum(&self) : int {
        return a + b;
    }

}

public func test_ext_pub_norm_inter_impl1() : int {
    var ext = ExtPubNormInterfaceImpl1 { a : 10, b : 20 }
    return ext.sum();
}

public struct ExtPubNormInterfaceImpl2 : ExtPubNormInterface {

    var a : int
    var b : int
    var c : int

    @override
    func sum(&self) : int {
        return a + b + c;
    }

}

public func test_ext_pub_norm_inter_impl2() : int {
    var ext = ExtPubNormInterfaceImpl2 { a : 10, b : 20, c : 30 }
    return ext.sum();
}