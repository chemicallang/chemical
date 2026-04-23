variant ForLoopPattMatchContinue {
    Some(value : int)
    None()
}

func test_for_loop() {

    test("continue in for loop, increments", () => {
        var counter = 0;
        var safety = 0
        for (var i = 0; i < 33; i++) {
            counter++;
            // Safety check: if loop body runs too many times, something is wrong
            safety++
            if (safety > 100) {
                return false;
            }
            if (i == 2) {
                continue; // should still let i++ happen
            }
        }

        // Correct behavior: loop runs exactly 33 times
        return (counter == 33);
    })

    test("continue in for loop with pattern matching works", () => {
        var counter = 0;
        var safety = 0
        var something = ForLoopPattMatchContinue.None()
        for (var i = 0; i < 33; i++) {
            counter++;
            // Safety check: if loop body runs too many times, something is wrong
            safety++
            if (safety > 100) {
                return false;
            }
            if (i == 2) {
                var Some(value) = something else continue; // should still let i++ happen
            }
        }

        // Correct behavior: loop runs exactly 33 times
        return (counter == 33);
    })
    test("loop continue and break work", () => {
        var i = 0;
        for(var j = 0; j < 10; j++) {
            for(var x = 0; x < 5; x++) {
                if(x == 3) {
                    break;
                }
                i++;
            }
            if(j == 7) {
                break;
            }
        }
        return i == 24;
    })
    test("loop continue and break work with pattern matching", () => {
        var something = ForLoopPattMatchContinue.None()
        var i = 0;
        for(var j = 0; j < 10; j++) {
            for(var x = 0; x < 5; x++) {
                if(x == 3) {
                    // will always cause a break here
                    var Some(value) = something else break;
                }
                i++;
            }
            if(j == 7) {
                // will always cause a break here
                var Some(value) = something else break;
            }
        }
        return i == 24;
    })

}