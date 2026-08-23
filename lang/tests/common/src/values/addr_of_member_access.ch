// Tests for (&raw expr).member and (&mut expr).member access chain patterns
// These test the C codegen fix where &expr->member was incorrectly generated
// instead of (&expr)->member (C operator precedence bug)

struct RawAccessPoint {
    var x : int
    var y : int
}

func get_raw_access_x(p : *RawAccessPoint) : int {
    return p.x
}

func set_raw_access_y(p : *mut RawAccessPoint, val : int) {
    p.y = val
}

func test_addr_of_member_access() {

    // --- &raw struct member access ---

    test("&raw struct var member read works", () => {
        var s = RawAccessPoint { x : 10, y : 20 }
        return (&raw s).x == 10
    })

    test("&raw struct var member read second field works", () => {
        var s = RawAccessPoint { x : 10, y : 20 }
        return (&raw s).y == 20
    })

    // --- &raw array element member access ---

    test("&raw array element member read works", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 1, y : 2 }
        arr[1] = RawAccessPoint { x : 10, y : 20 }
        arr[2] = RawAccessPoint { x : 100, y : 200 }
        var i : int = 1
        return (&raw arr[i]).x == 10
    })

    test("&raw array element member read second field works", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 1, y : 2 }
        arr[1] = RawAccessPoint { x : 10, y : 20 }
        arr[2] = RawAccessPoint { x : 100, y : 200 }
        var i : int = 2
        return (&raw arr[i]).y == 200
    })

    // --- &raw in if condition (the bug case) ---

    test("&raw struct member in if condition works", () => {
        var s = RawAccessPoint { x : 42, y : 0 }
        if((&raw s).x == 42) {
            return true
        }
        return false
    })

    test("&raw array element member in if condition works", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 0, y : 0 }
        arr[1] = RawAccessPoint { x : 77, y : 0 }
        arr[2] = RawAccessPoint { x : 0, y : 0 }
        var i : int = 1
        if((&raw arr[i]).x == 77) {
            return true
        }
        return false
    })

    test("&raw struct bool member in if condition works", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 0, y : 0 }
        arr[1] = RawAccessPoint { x : 7, y : 0 }
        arr[2] = RawAccessPoint { x : 0, y : 0 }
        var i : int = 1
        // non-zero x is truthy in C
        if((&raw arr[i]).x) {
            return true
        }
        return false
    })

    // --- &raw in while condition ---

    test("&raw member in while loop condition works", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 5, y : 0 }
        arr[1] = RawAccessPoint { x : 5, y : 0 }
        arr[2] = RawAccessPoint { x : 0, y : 0 }
        var sum : int = 0
        var i : int = 0
        while((&raw arr[i]).x > 0) {
            sum = sum + (&raw arr[i]).x
            i++
        }
        return sum == 10
    })

    // --- &raw in ternary/conditional expression ---

    test("&raw member in conditional expression works", () => {
        var s = RawAccessPoint { x : 99, y : 1 }
        var result = if((&raw s).x > 50) 1 else 0
        return result == 1
    })

    // --- passing &raw expr result to function ---

    test("&raw struct passed to function works", () => {
        var s = RawAccessPoint { x : 55, y : 66 }
        return get_raw_access_x(&raw s) == 55
    })

    // --- &raw in assignment RHS ---

    test("&raw member assigned to variable works", () => {
        var s = RawAccessPoint { x : 33, y : 44 }
        var val = (&raw s).x
        return val == 33
    })

    test("&raw array element member assigned to variable works", () => {
        unsafe var arr : [2]RawAccessPoint
        arr[0] = RawAccessPoint { x : 11, y : 22 }
        arr[1] = RawAccessPoint { x : 33, y : 44 }
        var i : int = 0
        var val = (&raw arr[i]).y
        return val == 22
    })

    // --- &raw member used in arithmetic ---

    test("&raw member in arithmetic expression works", () => {
        var s = RawAccessPoint { x : 10, y : 20 }
        return (&raw s).x + (&raw s).y == 30
    })

    // --- chained &raw array element access ---

    test("multiple &raw array element accesses in same expression", () => {
        unsafe var arr : [3]RawAccessPoint
        arr[0] = RawAccessPoint { x : 1, y : 2 }
        arr[1] = RawAccessPoint { x : 10, y : 20 }
        arr[2] = RawAccessPoint { x : 100, y : 200 }
        var i : int = 0
        var j : int = 2
        return (&raw arr[i]).x + (&raw arr[j]).y == 201
    })
}
