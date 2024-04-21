import "test.ch"

func capturing(lambda : []() => bool) {
    return lambda();
}

func dontCapture(lambda : () => bool) {
    return lambda();
}

func test_lambda() {
    test("testing non capturing lambda works", []() => {
        return true;
    })
    test("testing lambda without braces works", []() => true)
    test("testing non capturing lambda works", []() => {
        return dontCapture(() => {
            return true;
        });
    });
    test("testing non capturing lambda works without body", []() => {
        return dontCapture(() => true);
    });
    test("testing capturing & nested lambda works", []() => {
        return capturing([]() => {
            return true;
        });
    });
}