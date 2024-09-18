import "../../../std/vector.ch"

struct VecConcreteChild {
    var a : int
    var b : int
}

func test_vectors() {
    test("vector of ints work", () => {
        var v = vector<int>();
        v.push(10);
        v.push(20);
        return v.size() == 2 && v.get(0) == 10 && v.get(1) == 20
    })
    test("vector of longs work", () => {
        var v = vector<long>();
        v.push(10);
        v.push(20);
        return v.size() == 2 && v.get(0) == 10 && v.get(1) == 20
    })
    test("can remove last element from vector", () => {
        var v = vector<int>();
        v.push(10);
        v.push(20);
        v.remove_last();
        return v.size() == 1 && v.get(0) == 10;
    })
    test("can remove first element from vector", () => {
        var v = vector<int>();
        v.push(10);
        v.push(20);
        v.remove(0);
        return v.size() == 1 && v.get(0) == 20;
    })
    test("vectors work even when pushing large amount of items", () => {
        var v = vector<int>();
        var i = 10;
        var sum = 0;
        while(i < 300) {
            v.push(10);
            sum += 10;
            i += 10;
        }
        var act_sum = 0;
        i = 0;
        while(i < v.size()) {
            act_sum += v.get(i)
            i++;
        }
        return sum == act_sum;
    })
    test("test can change vector elements at index", () => {
        var v = vector<int>();
        v.push(10);
        v.push(20);
        v.push(30);
        v.push(40);
        v.push(50);
        v.set(0, 50);
        v.set(1, 40);
        v.set(2, 30);
        v.set(3, 20);
        v.set(4, 10);
        return v.get(0) == 50 && v.get(1) == 40 && v.get(2) == 30 && v.get(3) == 20 && v.get(4) == 10;
    })
    /**
    test("struct types can be stored in vector", () => {
        var v = vector<VecConcreteChild>();
        v.push(VecConcreteChild { a : 40, b : 50 })
        v.push(VecConcreteChild { a : 10, b : 3 })
        v.push(VecConcreteChild { a : 67, b : 232 })
        const first = v.get_ptr(0)
        const second = v.get_ptr(1)
        const third = v.get_ptr(2)
        return first.a == 40 && first.b == 50 && second.a == 10 && second.b == 3 && third.a == 67 && third.b == 232
    })
    **/
}