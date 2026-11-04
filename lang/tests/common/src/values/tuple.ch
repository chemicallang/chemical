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

struct TupleStructPoint(int, int)
struct TupleStructExternalMethods(int, int)
impl TupleStructExternalMethods {
    func product(&self) : int {
        return self._0 * self._1
    }
}
struct TupleStructPointMethods(int, int) {
    func sum(&self) : int {
        return self._0 + self._1
    }
}
struct TupleStructGeneric<T>(T, T)
struct TupleStructGenericMethods<T>(T, T) {
    func count(&self) : int { return 2 }
    func first(&self) : T where T: Copy { return self._0 }
}
struct TupleStructDrops(TupleDropProbe, TupleDropProbe)

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
    test("tuple destructuring binds a snapshot", () => {
        var pair = (1, 2)
        var (a, b) = pair
        pair._0 = 9
        pair._1 = 8
        return a == 1 && b == 2
    })
    test("tuple destructuring from a call and one-element pattern", () => {
        var (a, b) = tuple_pair(7, true)
        const (c,) = (9,)
        var (_, only) = (11, 12)
        return a == 7 && b && c == 9 && only == 12
    })
    test("tuple destructuring moves destructible elements exactly once", () => {
        var first = 0
        var second = 0
        {
            var pair = (TupleDropProbe { n : 1, counter : &mut first },
                        TupleDropProbe { n : 2, counter : &mut second })
            var (left, right) = pair
            if(left.n != 1 || right.n != 2) {
                return false
            }
        }
        return first == 1 && second == 1
    })
    test("nested tuple destructuring", () => {
        var value = ((1, 2), 3)
        var ((a, b), c) = value
        var (_, (d, e)) = ((9, 10), (11, 12))
        return a == 1 && b == 2 && c == 3 && d == 11 && e == 12
    })
    test("tuple pattern in if condition", () => {
        var value = (1, (2, 3))
        if(var (a, (b, c)) = value) {
            return a + b + c == 6
        }
        return false
    })
    test("nested tuple destructuring moves destructible elements exactly once", () => {
        var first = 0
        var second = 0
        var third = 0
        var fourth = 0
        {
            var value = ((TupleDropProbe { n : 1, counter : &mut first },
                          TupleDropProbe { n : 2, counter : &mut second }),
                         (TupleDropProbe { n : 3, counter : &mut third },
                          TupleDropProbe { n : 4, counter : &mut fourth }))
            var ((a, b), (c, d)) = value
            if(a.n != 1 || b.n != 2 || c.n != 3 || d.n != 4) {
                return false
            }
        }
        return first == 1 && second == 1 && third == 1 && fourth == 1
    })
    test("tuple struct construction and field access", () => {
        var point = TupleStructPoint(3, 4)
        var explicit : TupleStructPoint = TupleStructPoint(5, 6)
        return point._0 == 3 && point._1 == 4 &&
               explicit._0 == 5 && explicit._1 == 6
    })
    test("tuple struct destructuring", () => {
        var (a, b) = TupleStructPoint(7, 8)
        return a == 7 && b == 8
    })
    test("generic tuple struct construction and destructuring", () => {
        var pair = TupleStructGeneric<int>(9, 10)
        var (a, b) = pair
        return a == 9 && b == 10
    })
    test("tuple struct inline methods", () => {
        var point = TupleStructPointMethods(3, 4)
        var pair = TupleStructGenericMethods<int>(5, 6)
        return point.sum() == 7 && pair.count() == 2 && pair.first() == 5
    })
    test("tuple struct external inherent methods", () => {
        var point = TupleStructExternalMethods(3, 4)
        return point.product() == 12
    })
    test("tuple struct destructible elements drop exactly once", () => {
        var first = 0
        var second = 0
        {
            var pair = TupleStructDrops(
                TupleDropProbe { n : 1, counter : &mut first },
                TupleDropProbe { n : 2, counter : &mut second })
            var (left, right) = pair
            if(left.n != 1 || right.n != 2) {
                return false
            }
        }
        return first == 1 && second == 1
    })
    test("unit tuple value and type", () => {
        var unit : () = ()
        var copy = unit
        return sizeof(()) == 0
    })
    test("array patterns bind exact and rest elements", () => {
        var values : [4]int = [1, 2, 3, 4]
        var [a, b, c, d] = values
        var [first, .., last] = values
        var [.., tail] = values
        var [head, ..] = values
        var [_, x, _, y] = values
        values[0] = 99
        return a == 1 && b == 2 && c == 3 && d == 4 &&
               first == 1 && last == 4 && tail == 4 && head == 1 &&
               x == 2 && y == 4
    })
    test("array patterns bind Copy aggregate elements", () => {
        var pairs : [2](int, int) = [(1, 2), (3, 4)]
        var [left, right] = pairs
        pairs[0]._0 = 9
        return left._0 == 1 && left._1 == 2 &&
               right._0 == 3 && right._1 == 4
    })
}
