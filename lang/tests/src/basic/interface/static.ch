import "@submod/interface/external.ch"

@static
interface StaticSummer {

    func sum(&self) : int

}

func (summer : &mut StaticSummer) multiplied_sum() : int {
    return summer.sum() * 2;
}

struct ImplStaticSummer : StaticSummer {

    var a : int
    var b : int

    @override
    func sum(&self) : int {
        return a + b + 2;
    }

}

@static
public interface ExportedStaticMultiplier {

    func multiply(&self) : int

}

public func (multiplier : &mut ExportedStaticMultiplier) double_multiplied() : int {
    return multiplier.multiply() + multiplier.multiply()
}

public struct ImplExportedStaticMultiplier : ExportedStaticMultiplier {

    var a : int
    var b : int

    @override
    func multiply(&self) : int {
        return a * b;
    }

}

func (thing : &mut ExternallyImplementedInterface) get_interface_num_in_curr_mod() : int {
    return thing.give_number();
}

struct RightExternalNumber : ExternallyImplementedInterface {

    @override
    func give_number(&self) : int {
        return 8765;
    }

}

func test_static_interfaces() {

    test("methods in static interfaces work", () => {
        var summer = ImplStaticSummer { a : 10, b : 10 }
        return summer.sum() == 22;
    })

    test("methods of static interfaces are callable in extension functions", () => {
        var summer = ImplStaticSummer { a : 10, b : 10 }
        return summer.multiplied_sum() == 44;
    })

    test("methods in public static interfaces work", () => {
        var multiplier = ImplExportedStaticMultiplier { a : 10, b : 10 }
        return multiplier.multiply() == 100;
    })

    test("methods of public static interfaces are callable in extension functions", () => {
        var multiplier = ImplExportedStaticMultiplier { a : 10, b : 10 }
        return multiplier.double_multiplied() == 200;
    })

    test("externally implemented imported interface works", () => {
        var thing = CurrModPubIntImpl {}
        return thing.give_number() == 8787
    })

    test("externally implemented imported interface works through extension method", () => {
        var thing = CurrModPubIntImpl {}
        return thing.inc_imp_pub_int() == 8788
    })

    test("external interfaces implemented in current module work", () => {
        var thing = RightExternalNumber {  }
        return thing.give_number() == 8765
    })

    test("external interfaces implemented in current module work", () => {
        var thing = RightExternalNumber {  }
        return thing.give_number() == 8765
    })

    test("external interfaces implemented in current module work through extension method in current module", () => {
        var thing = RightExternalNumber {  }
        return thing.get_interface_num_in_curr_mod() == 8765
    })

    test("external interfaces implemented in current module work through extension method in external module", () => {
        var thing = RightExternalNumber {  }
        return thing.pls_give_ext_num() == 8765
    })

}