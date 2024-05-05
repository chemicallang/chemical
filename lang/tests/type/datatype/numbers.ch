import "../../test.ch"

func test_numbers() {
    test("test unsigned int works", () => {
        var i : uint = 33;
        var w : uint = 33;
        return i == w;
    })
    test("test short works", () => {
        var i : short = 22;
        var w : short = 22;
        return i == w;
    })
    test("test unsigned short works", () => {
        var i : ushort = 44;
        var w : ushort = 44;
        return i == w;
    })
    test("test long works", () => {
        var i : long = 777;
        var w : long = 777;
        return i == w;
    })
    test("test unsigned long works", () => {
        var i : ulong = 777;
        var w : ulong = 777;
        return i == w;
    })
    test("test big integer works", () => {
        var i : bigint = 888;
        var w : bigint = 888;
        return i == w;
    })
    test("test unsigned big integer works", () => {
        var i : ubigint = 999;
        var w : ubigint = 999;
        return i == w;
    })
    test("test unsigned int demotes int32 to uint", () => {
        var i : uint = 1;
        return i == 1;
    })
    test("test short comparison demotes int32 to short", () => {
        var i : short = 1;
        return i == 1;
    })
    test("test short comparison demotes int32 to short", () => {
        var i : short = 1;
        return i == 1;
    })
    test("test ushort comparison demotes int32 to ushort", () => {
        var i : ushort = 1;
        return i == 1;
    })
    /**
    test("test constant long is demoted to int32 for comparison with int32", () => {
        return 123L == 123;
    })
    **/
}