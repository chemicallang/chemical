func tuple_pair(a : int, b : bool) : (int, bool) {
    return (a, b)
}

func tuple_sum(pair : (int, int)) : int {
    return pair._0 + pair._1
}

func <T> tuple_copy_identity(value : T) : T where T: Copy {
    return value
}

struct TupleDropProbe {
    var n : int
    var counter : &mut int

    @delete
    func delete(&mut self) {
        *self.counter += 1
    }
}

func test_tuples() {
    test("tuple literal construction and element access", () => {
        var value = (1, true)
        return value._0 == 1 && value._1
    })
    test("tuple values pass through function parameters and returns", () => {
        var pair = tuple_pair(7, true)
        return pair._0 == 7 && pair._1 && tuple_sum((4, 5)) == 9
    })
    test("nested tuple element access", () => {
        var nested = ((1, 2), (3, 4))
        return nested._0._1 == 2 && nested._1._0 == 3
    })
    test("tuple satisfies Copy when all elements do", () => {
        var value = tuple_copy_identity((11, 12))
        return value._0 == 11 && value._1 == 12
    })
    test("tuple element assignment", () => {
        var value : (int, bool) = (1, false)
        value._0 = 9
        value._1 = true
        return value._0 == 9 && value._1
    })
    test("tuple elements are dropped exactly once", () => {
        var first = 0
        var second = 0
        {
            var value = (TupleDropProbe { n : 1, counter : &mut first },
                         TupleDropProbe { n : 2, counter : &mut second })
            if(value._0.n != 1 || value._1.n != 2) {
                return false
            }
        }
        return first == 1 && second == 1
    })
}
