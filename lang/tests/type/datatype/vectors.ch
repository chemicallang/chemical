import "../../../std/vector.ch"

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
}