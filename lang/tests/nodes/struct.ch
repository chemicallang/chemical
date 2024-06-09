import "../test.ch"

/** TODO are we going to support this syntax
struct Something {

    func do() : bool;

}

impl Something {
    func do() : bool {
        return true;
    }
}
**/

struct Pair {
    var a : int
    var b : int
}

func create_pair() : Pair {
    return Pair {
        a : 33,
        b : 55
    }
}

func create_pair_as_variable() : Pair {
    var p = Pair {
        a : 44,
        b : 66
    }
    return p;
}

func test_structs() {
    /**
    test("can return a newly created struct", () => {
        var pair = create_pair();
        return pair.a == 33 && pair.b == 55;
    })
    test("can return a newly created struct that is referenced", () => {
        var pair = create_pair_as_variable();
        return pair.a == 44 && pair.b == 66;
    })
    test("can call declared in struct and impl", () => {
        return Something.do() == true;
    });
    **/
}