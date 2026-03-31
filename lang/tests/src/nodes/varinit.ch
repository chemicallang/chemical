comptime const glob_ct_const = 100 + 300

const glob_const = 800

struct TLInherited {
    var a : int = 10
}

struct TLInheriter : TLInherited {
    var b : int = 20
}

var struct_inheriter_variable = TLInheriter { b : 33 }
var struct_inheriter_variable2 = TLInheriter { TLInherited : TLInherited { a : 323 }, b : 36 }

variant TLInheriterVariant : TLInherited {
    None()
    Some(x : int)
}

var variant_inheriter_variable = TLInheriterVariant.None()
var variant_inheriter_variable2 = TLInheriterVariant.Some(837)

struct GlobalVariableAutoZero {
    var a : int;
    var b : int
}

var auto_zero_variable : GlobalVariableAutoZero

namespace wrapped_variable {
    const thing = 3456
}

func test_var_init() {
    test("wrapped variable in namespace works", () => {
        var x = wrapped_variable::thing;
        return x == 3456
    })
    test("non extern global variables are initialized to zero by default when no initializer", () => {
        return auto_zero_variable.a == 0 && auto_zero_variable.b == 0
    })
    test("structs inherited in top level variables initialized to default values if no value given", ()=> {
        return struct_inheriter_variable.a == 10;
    })
    test("structs inherited in top level variables initialized to given value", ()=> {
        return struct_inheriter_variable2.a == 323;
    })
    test("structs in top level variables work - 1", ()=> {
        return struct_inheriter_variable.b == 33
    })
    test("structs in top level variables work - 2", ()=> {
        return struct_inheriter_variable2.b == 36
    })
    test("variant inherited in top level variable initialized to default value", () => {
        return variant_inheriter_variable.a == 10
    })
    test("variant in top level variables work - 1", () => {
        switch(variant_inheriter_variable) {
            None => { return true; }
            default => { return false; }
        }
    })
    test("variant in top level variables work - 2", () => {
        switch(variant_inheriter_variable2) {
            None => { return false; }
            Some(x) => { return x == 837 }
            default => { return false; }
        }
    })
    test("can initialize normal variables", () => {
        var x = 5;
        return x == 5;
    })
    test("can modify variables", () => {
        var x = 5;
        x = 6;
        return x == 6;
    });
    test("can assign to a non initialized variable", () => {
        var x : int
        x = 6;
        return x == 6;
    })
    test("can initialize a typed variable", () => {
        var x : int = 5;
        return x == 5;
    })
    test("global comptime constants work", () => {
        return glob_ct_const == 400;
    })
    test("global constants work", () => {
        return glob_const == 800;
    })
    test("local constants work as well", () => {
        const something = 1200 + 400
        return something == 1600
    })
    test("global variable structs can be passed as default values to functions above them", () => {
        return take_global_variable_as_default_value() == 256;
    })
}

struct GlobalVariablePassedAsDefaultValue {
    var a : int
    var b : int
}

func take_global_variable_as_default_value(p : &GlobalVariablePassedAsDefaultValue = global_var_def_value_struct) : int {
    return p.a + p.b
}

var global_var_def_value_struct = GlobalVariablePassedAsDefaultValue { a : 234, b : 22 }