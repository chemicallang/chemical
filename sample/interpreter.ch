struct wow {
    var i : int
}

func modify(x : wow) {
    x.i = 6;
}

// interpreter copied value from AST
func copied_return() {
    return 123;
}

// interpreter moved the value
func moved_return() {
    var x = 345; // copied from AST here
    return x; // moved here
}

func struct_references() {
    var x = wow {
        i : 5
    };
    // pass by reference
    modify(x);
    if(x.i == 6){
        // another reference
        var y = x;
        // scope ended, but y is a reference, so x won't be destroyed
    }
    return x;
}

func copy_above_primitive() {
    var x : int
    if(true) {
        x = 5;
    }
    return x;
}

func copy_above_primitive_by_id() {
    var x : int
    if(true) {
        var y = 5;
        x = y;
    }
    return x;
}

func moving_above_struct_value() {
    var x : wow;
    if(5 == 5) {
        x = wow {
            i : 7
        };
    }
    return x;
}

func moving_struct_down() {
    var x : wow = wow {
        i : 0
    }
    if(true) {
        // testing with declaration
        var z : wow
        z = x;
        // testing with initialization
        var j = x;
        j = x;
    }
    return x;
}

func moving_above() {
    var x : wow = wow {
        i : 0
    }
    if(true) {
        var z = wow {
            i : 16
        }
        // moving z to x, as x was declared above z (not yet working)
        x = z
    }
    return x;
}

func main() : int {
    print("copied-return:", copied_return(), '\n');
    print("moved-return:", moved_return(), '\n');
    print("copy-above-primitive:", copy_above_primitive(), '\n');
    print("copy-above-primitive-by-id:", copy_above_primitive_by_id(), '\n');
    print("struct-references:", struct_references(), '\n');
    print("moving-above-struct-value:", moving_above_struct_value(), '\n');
    print("moving_struct_down:", moving_struct_down(), '\n');
    print("moving-above:", moving_above(), '\n');
    return 0;
}

var __main__ = main();