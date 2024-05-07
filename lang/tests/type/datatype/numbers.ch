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
    test("test constant long is demoted to int32 for comparison with int32", () => {
        return 123 == 123L;
    })
    test("test short can be assigned", () => {
        var i : short = 44;
        i = 33;
        return i == 33;
    })
    test("test int can be assigned int max", () => {
        var i : int = 10;
        i = 2147483647;
        return i == 2147483647;
    })
    // TODO long is 32bit on 64bit windows
    // TODO to test long for 64bit a larger max value should be used
    // TODO a larger max value for long only should be used when it's bit size is larger than 32bit
    // TODO to check that we must develop sizeof macro, we'll also develop bitsizeof
    test("test long can be assigned long max", () => {
        var i : long = 10;
        i = 2147483647;
        return i == 2147483647;
    })
    test("test bigint can be assigned bigint max", () => {
        var i : bigint = 10;
        i = 9223372036854775807;
        return i == 9223372036854775807;
    })
    test("can promote int constants to float when compared with float variable", () => {
        var i : float = 10.0f;
        return i == 10;
    })
    test("can promote int constants to double when compared with double variable", () => {
        var i : double = 10.0;
        return i == 10;
    })
    test("can extend int n type to compare with a different bit type", () => {
        var i : short = 5;
        var w : int = 5;
        return i == w;
    })
}