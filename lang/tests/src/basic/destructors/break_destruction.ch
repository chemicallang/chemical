func test_break_destruction() {
    test("break in a loop destructs objects in its scope", () => {
        var count = 0
        for(var i = 0; i < 2; i++) {
            if(i == 0) {
                var d = Destructible {
                    data : 739, count : &mut count, lamb : destruct_inc_count
                }
                break;
            }
        }
        return count == 1
    })
    test("break in a loop destructs objects in loop scope", () => {
        var count = 0
        for(var i = 0; i < 10; i++) {
            var d = Destructible {
                data : 739, count : &mut count, lamb : destruct_inc_count
            }
            if(i == 0) {
                break;
            } else {
                return false;
            }
        }
        return count == 1
    })
    test("break in a loop does NOT destruct objects above loop", () => {
        var count = 0
        var d = Destructible {
            data : 739, count : &mut count, lamb : destruct_inc_count
        }
        for(var i = 0; i < 2; i++) {
            if(i == 0) {
                break;
            }
        }
        return count == 0
    })
    test("break doesn't cause failure in compilation", () => {
        var count = 0;
        var i = 0
        loop {
            if(i == 2) {
                var d = Destructible {
                    data : 739, count : &mut count, lamb : destruct_inc_count
                }
                break;
            }
            i++
        }
        return count == 1
    })
}