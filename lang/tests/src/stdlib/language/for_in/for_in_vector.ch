struct ForInVecPoint {
    var i : int

    func get_i(&self) : int {
        return i;
    }

}

variant ForInPattMatchVec {
    Some(value : int)
    None()
}

func test_for_in_vector() {
    test("can iterate over vector", () => {
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i in s) {
            total += i
        }
        return total == 60
    })
    test("can iterate over vector in reverse", () => {
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i in s reversed) {
            total += i
        }
        return total == 60
    })
    test("continue using pattern match, for in iterate over vector", () => {
        var p = ForInPattMatchVec.None()
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i, j in s) {
            if(j == 2) {
                var Some(value) = p else continue;
            }
            total += i
        }
        return total == 50
    })
    test("break using pattern match, for in iterate over vector", () => {
        var p = ForInPattMatchVec.None()
        var s = std::vector<int>()
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var i, j in s) {
            if(j == 2) {
                var Some(value) = p else break;
            }
            total += i
        }
        return total == 20
    })
    test("can iterate over vector using a reference", () => {
        var s = std::vector<int>()
        s.push(20);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var& i in s) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over vector using a reference in reverse", () => {
        var s = std::vector<int>()
        s.push(20);
        s.push(10);
        s.push(10);
        s.push(10);
        s.push(20);
        var total = 0;
        for(var& i in s reversed) {
            total += *i
        }
        return total == 70
    })
    test("can iterate over vector of struct", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var i in s) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over vector of struct in reverse", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var i in s reversed) {
            total += i.i
        }
        return total == 60
    })
    test("can iterate over vector of struct using a reference", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 20 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var& i in s) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate over vector of struct using a reference in reverse", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 20 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var& i in s reversed) {
            total += i.i
        }
        return total == 70
    })
    test("can iterate & call method over vector of struct", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var i in s) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over vector of struct in reverse", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var i in s reversed) {
            total += i.get_i()
        }
        return total == 60
    })
    test("can iterate & call method over vector of struct using a reference", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 20 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var& i in s) {
            total += i.get_i()
        }
        return total == 70
    })
    test("can iterate & call method over vector of struct using a reference in reverse", () => {
        var s = std::vector<ForInVecPoint>()
        s.push(ForInVecPoint { i : 20 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 10 });
        s.push(ForInVecPoint { i : 20 });
        var total = 0;
        for(var& i in s reversed) {
            total += i.get_i()
        }
        return total == 70
    })
}