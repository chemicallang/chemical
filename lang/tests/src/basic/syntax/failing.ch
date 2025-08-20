func failing_socketpair(i : int) : int {
    return i;
}

func test_failing_code() {

    test("parsing succeeds on failing code - 1", () => {
        var sv : int[2]
        if(failing_socketpair(0) < 0) {
            return false;
        }
        return true;
    })

}