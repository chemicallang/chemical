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

}