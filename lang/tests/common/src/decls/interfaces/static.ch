
@static
interface StaticSummer {

    func sum(&self) : int

}

func (summer : &mut StaticSummer) multiplied_sum() : int {
    return summer.sum() * 2;
}

struct ImplStaticSummer {

    var a : int
    var b : int

}

impl StaticSummer for ImplStaticSummer {
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

public struct ImplExportedStaticMultiplier {

    var a : int
    var b : int


}

impl ExportedStaticMultiplier for ImplExportedStaticMultiplier {
    func multiply(&self) : int {
        return a * b;
    }
}

@static
interface StructRetStaticInter {

    func provide(&self) : ImplStaticSummer

}

func (inter : &mut StructRetStaticInter) sum_provider() : int {
    var p = inter.provide();
    return p.a + p.b
}

struct StructRetStaticInterImpl {

    var x : int
    var y : int

}

impl StructRetStaticInter for StructRetStaticInterImpl {
    func provide(&self) : ImplStaticSummer {
        return ImplStaticSummer { a : x, b : y}
    }
}

struct MultiplierBeforeInterface {
    var a : int
    var b : int
}

impl StaticMultiplierInterface for MultiplierBeforeInterface {
    func multiply(&self) : int {
        return a * b;
    }
}

@static
interface StaticMultiplierInterface {
    func multiply(&self) : int
}

impl StaticDividerInterface for DividableStructForStaticInterface {
    func divide(&self) : int {
        return a / b;
    }
}

struct DividableStructForStaticInterface {
    var a : int
    var b : int
}

@static
interface StaticDividerInterface {
    func divide(&self) : int
}

func test_static_interfaces() {

    test("methods in static interfaces work", () => {
        var summer = ImplStaticSummer { a : 10, b : 10 }
        return summer.sum() == 22;
    })

    test("methods of static interfaces are callable in extension functions", () => {
        var summer = ImplStaticSummer { a : 10, b : 10 }
        return (summer as StaticSummer).multiplied_sum() == 44;
    })

    test("methods in public static interfaces work", () => {
        var multiplier = ImplExportedStaticMultiplier { a : 10, b : 10 }
        return multiplier.multiply() == 100;
    })

    test("methods of public static interfaces are callable in extension functions", () => {
        var multiplier = ImplExportedStaticMultiplier { a : 10, b : 10 }
        return (multiplier as ExportedStaticMultiplier).double_multiplied() == 200;
    })

    test("methods of static interfaces can return structs", () => {
        var thing = StructRetStaticInterImpl { x : 32, y : 87 };
        return (thing as StructRetStaticInter).sum_provider() == 32 + 87;
    })

    test("static interface can come after implementation - 1", () => {
        var p = MultiplierBeforeInterface { a : 3, b : 9 }
        return p.multiply() == 27
    })

    test("static interface can come after implementation - 2", () => {
        var p = DividableStructForStaticInterface { a : 66, b : 6 }
        return p.divide() == 11
    })

}