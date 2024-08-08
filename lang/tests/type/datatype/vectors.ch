import "../../../std/vector.ch"

func test_vectors() {
    test("vector of ints work", () => {
        var v = vector<int>();
        v.push(10);
        v.push(20);
        return v.size() == 2 && v.get(0) == 10 && v.get(1) == 20
    })
}