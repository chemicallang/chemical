variant PMOpt1 {
    Some(value : int)
    None()
}

variant PMMultiOpt {
    Some(value1 : int, value2 : int)
    None()
}

func test_variant_pattern_matching() {
    test("variant pattern matching with unreachable works", () => {
        var opt = PMOpt1.Some(10)
        var Some(value) = opt else unreachable;
        return value == 10
    })
    test("variant pattern matching with default value works - 1", () => {
        var opt2 = PMOpt1.Some(20)
        var Some(value) = opt2 else 10
        return value == 20;
    })
    test("variant pattern matching with default value works - 2", () => {
        var opt2 = PMOpt1.None()
        var Some(value) = opt2 else 10
        return value == 10;
    })
    test("variant pattern matching with return works - 1", () => {
        var opt3 = PMOpt1.Some(15)
        var Some(value) = opt3 else return 10;
        return value == 15;
    })
    test("variant pattern matching with return works - 2", () => {
        var opt3 = PMOpt1.None()
        var Some(value) = opt3 else return true;
        return false;
    })
    test("variant pattern matching with multiple values work - 1", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value1, value2) = opt else unreachable
        return value1 == 10 && value2 == 20
    })
    test("variant pattern matching with multiple values work - 2", () => {
        var opt = PMMultiOpt.Some(10, 20)
        var Some(value1, value2) = opt else return false;
        return value1 == 10 && value2 == 20
    })
    test("variant pattern matching with multiple values work - 3", () => {
        var opt = PMMultiOpt.None()
        var Some(value1, value2) = opt else return true;
        return false
    })
    test("variant pattern matching with if work - 1", () => {
        var s = PMOpt1.Some(78)
        if(var Some(value) = s) {
            return value == 78
        } else {
            return false;
        }
    })
    test("variant pattern matching with if work - 2", () => {
        var s = PMOpt1.None()
        if(var Some(value) = s) {
            return false
        } else {
            return true;
        }
    })
}