func test_continue_destruction() {
    test("continue in a loop destructs objects in its scope", () => {
        var count = 0
        for(var i = 0; i < 2; i++) {
            if(i == 0) {
                var d = Destructible {
                    data : 739, count : &mut count, lamb : destruct_inc_count
                }
                continue;
            }
        }
        return count == 1
    })
    test("continue in a loop destructs objects in loop scope", () => {
        var count = 0
        for(var i = 0; i < 2; i++) {
            var d = Destructible {
                data : 739, count : &mut count, lamb : destruct_inc_count
            }
            if(i == 0) {
                continue;
            }
            if(i == 1 && count != 1) {
                return false;
            }
        }
        return count == 2
    })
    test("continue in a loop does NOT destruct objects above loop", () => {
        var count = 0
        var d = Destructible {
            data : 739, count : &mut count, lamb : destruct_inc_count
        }
        for(var i = 0; i < 2; i++) {
            if(i == 0) {
                continue;
            }
        }
        return count == 0
    })
}