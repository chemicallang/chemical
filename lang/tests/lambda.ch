import "test.ch"

func capturing(lambda : []() => bool) {
    return lambda();
}

func test_lambda() {
    test("testing non capturing lambda works", []() => {
        return true;
    })
    test("testing lambda without braces works", []() => true)
    test("testing capturing & nested lambda works", []() => {
        return capturing([]() => {
            return true;
        });
    });
}