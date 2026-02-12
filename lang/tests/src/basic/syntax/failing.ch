func failing_socketpair(i : int) : int {
    return i;
}

struct ParenExprAccessTest2 {
    var d : int
    func zero_d(&mut self) { d = 0; }
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

    test("parenthesized expressions as a statement work - 1", () => {
        var d = ParenExprAccessTest2 { d : 234 }
        var j = &d as *void
        (j as *mut ParenExprAccessTest2).d = 934
        return d.d == 934
    })

    test("parenthesized expressions as a statement work - 2", () => {
        var d = ParenExprAccessTest2 { d : 234 }
        var j = &d as *void
        (j as *mut ParenExprAccessTest2).zero_d()
        return d.d == 0
    })

    test("single statement block in if works - 1", () => {
        var j = true
        var x = 3
        if(j) x = 6;
        var f = 4
        if(!j) f = 7;
        return x == 6 && f == 4
    })

    test("single statement block in while works - 1", () => {
        var x = 0;
        while(x < 5) x++;
        return x == 5
    })

    test("single statement block in for works - 1", () => {
        var j = 2;
        for(var x = 0; x < 5; x++) j = x;
        return j == 4
    })

}