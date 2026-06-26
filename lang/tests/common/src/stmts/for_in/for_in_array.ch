struct ForInArrayPoint {
    var i : int

    func get_i(&self) : int {
        return i;
    }

}

variant ForInPattMatchArray {
    Some(value : int)
    None()
}

func test_for_in_array() {
    test("for in can iterate over array", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var total = 0;
        for(var i in arr) {
            total += i
        }
        return total == 60
    })
    test("for in can iterate over array in reverse", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var total = 0;
        for(var i in arr reversed) {
            total += i
        }
        return total == 60
    })
    test("continue using pattern match, for in iterate over array", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var total = 0;
        var p = ForInPattMatchArray.None()
        for(var i, j in arr) {
            if(j == 2) {
                var Some(value) = p else continue;
            }
            total += i
        }
        return total == 50
    })
    test("break using pattern match, for in iterate over array", () => {
        const arr : []int = [10, 10, 10, 10, 20]
        var total = 0;
        var p = ForInPattMatchArray.None()
        for(var i, j in arr) {
            if(j == 2) {
                var Some(value) = p else break;
            }
            total += i
        }
        return total == 20
    })
    test("for in can iterate over array using a reference", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var total = 0;
        for(var& i in arr) {
            total += *i
        }
        return total == 70
    })
    test("for in can iterate over array using a reference in reverse", () => {
        const arr : []int = [20, 10, 10, 10, 20]
        var total = 0;
        for(var& i in arr reversed) {
            total += *i
        }
        return total == 70
    })
    test("for in can iterate over array of struct", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 5 }, ForInArrayPoint { i : 15 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var i in arr) {
            total += i.i
        }
        return total == 60
    })
    test("for in can iterate over array of struct in reverse", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 5 }, ForInArrayPoint { i : 15 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var i in arr reversed) {
            total += i.i
        }
        return total == 60
    })
    test("for in can iterate over array of struct using a reference", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 20 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var& i in arr) {
            total += i.i
        }
        return total == 70
    })
    test("for in can iterate over array of struct using a reference in reverse", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 20 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var& i in arr reversed) {
            total += i.i
        }
        return total == 70
    })
    test("for in can iterate & call method over array of struct", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 5 }, ForInArrayPoint { i : 15 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var i in arr) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over array of struct in reverse", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 5 }, ForInArrayPoint { i : 15 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var i in arr reversed) {
            total += i.get_i()
        }
        return total == 60
    })
    test("for in can iterate & call method over array of struct using a reference", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 20 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var& i in arr) {
            total += i.get_i()
        }
        return total == 70
    })
    test("for in can iterate & call method over array of struct using a reference in reverse", () => {
        const arr : []ForInArrayPoint = [ ForInArrayPoint { i : 20 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 10 }, ForInArrayPoint { i : 20 }]
        var total = 0;
        for(var& i in arr reversed) {
            total += i.get_i()
        }
        return total == 70
    })
}