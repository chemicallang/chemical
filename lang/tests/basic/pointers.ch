import "../test.ch"

func test_pointer_math() : bool {
    test("testing basic pointer arithmetic", () => {
        var arr = {10, 20, 30, 40, 50};
        var ptr = &arr[0];
        var result1 = *ptr == 10;
        ptr++;
        var result2 = *ptr == 20;
        ptr++;
        var result3 = *ptr == 30;
        ptr--;
        var result4 = *ptr == 20;
        ptr++;
        ptr++;
        var result5 = *ptr == 40;
        ptr++;
        var result6 = *ptr == 50;
        return result1 && result2 && result3 && result4 && result5 && result6;
    });
    test("pointer subtraction works", () => {
        var arr = {10, 20, 30, 40, 50};
        var ptr1 = &arr[0] + 2;
        var ptr2 = &arr[0];
        var diff = ptr1 - ptr2;
        return diff == 2;
    })
}