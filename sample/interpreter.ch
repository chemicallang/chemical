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

func references() {
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

func moving_above() {
    var x : wow
    var y = 5
    if(y == 5) {
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
    print("references:", references(), '\n');
    print("moving-above", moving_above(), '\n');
    return 0;
}

var __main__ = main();