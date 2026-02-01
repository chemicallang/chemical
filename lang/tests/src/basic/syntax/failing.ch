func failing_socketpair(i : int) : int {
    return i;
}

struct ParenExprAccessTest2 {
    var d : int
}

func access_on_patenthesized_gbi(cc : int) : int {
    return cc;
}

func test_failing_code() {

    test("parsing succeeds on failing code - 1", () => {
        var sv : [2]int
        if(failing_socketpair(0) < 0) {
            return false;
        }
        return true;
    })

    test("access on parenthesized expression work - 1", () => {
        var d = ParenExprAccessTest2 { d : 87 }
        var j = &d as *void
        return access_on_patenthesized_gbi((j as *ParenExprAccessTest2).d) == 87
    })

    test("access on parenthesized expression work - 2", () => {
        var d = ParenExprAccessTest2 { d : 43 }
        var j = &d as *void
        if((j as *ParenExprAccessTest2).d == 43) {
            return true;
        } else {
            return false;
        }
    })

}