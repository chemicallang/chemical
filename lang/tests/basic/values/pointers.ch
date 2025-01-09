import "../../test.ch"

struct PMC22 {
    var a : int
    var b : int
}

func ret_ptr_to_int22(ptr : *mut int) : *mut int {
    return ptr;
}

func ret_ptr_to_struct22(ptr : *mut PMC22) : *mut PMC22 {
    return ptr;
}

func test_pointer_math() {
    test("assignment using a pointer works", () => {
        var i = 2;
        var j = &i;
        *j = *j + 1;
        return i == 3
    })
    test("double dereference works", () => {
        var i = 123;
        var j = &i
        var k = &j
        return **k == 123;
    })
    test("dereference of address works", () => {
        var i = 234;
        var k = *&i
        return k == 234;
    })
    test("address of dereference works", () => {
        var i = 345;
        var j = &i;
        var k = &*j;
        return *k == 345;
    })
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
    test("pointer comparison works - 1", () => {
        var arr = {10, 20, 30, 40, 50};
        var ptr1 = &arr[0] + 2;
        var ptr2 = &arr[0];
        return ptr1 > ptr2 && ptr2 < ptr1;
    })
    test("pointer comparison works - 2", () => {
        var arr = {10, 20, 30, 40, 50};
        var ptr1 = &arr[0] + 2;
        var ptr2 = &arr[0] + 2;
        return ptr1 == ptr2;
    })
    test("pointer comparison works - 3", () => {
        var arr = {10, 20, 30, 40, 50};
        var ptr1 = &arr[0] + 4;
        var ptr2 = &arr[0];
        return ptr2 < ptr1;
    })
    test("pointer access using index operator work", () => {
        var d : int[2]
        d[0] = 55;
        d[1] = 60
        var ptr = &d[0]
        return ptr[1] == 60;
    })
    test("can access children after doing pointer math", () => {
        var p = PMC22 { a : 22, b : 33 }
        const j = &p
        const k = j + 1;
        const d = k - 1;
        return d.a == 22 && d.b == 33;
    })
    test("can assign to a int pointer returned from function call", () => {
        var i = 20;
        *ret_ptr_to_int22(&i) = 20;
        return i == 20;
    })
    test("can assign to a struct pointer returned from a function call", () => {
        var p = PMC22 { a : 43, b : 87 }
        *ret_ptr_to_struct22(&p) = PMC22 { a : 32, b : 44 }
        return p.a == 32 && p.b == 44;
    })
}