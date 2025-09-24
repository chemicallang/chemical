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

}