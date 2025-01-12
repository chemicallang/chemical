import "../../test.ch"

func assign_to_addr(value : *int) {
    *value = 25
}

func take_addr_of(value : int) : int {
    assign_to_addr(&value)
    return value;
}

// func <T> gen_assign_to_addr(value : *T) {
//     if(T is short) {
//         *value = 2
//     } else if(T is int) {
//         *value = 4
//     } else if(T is bigint) {
//         *value = 8
//     } else {
//         value = 99
//     }
// }
//
// func <T> gen_take_addr_of(value : T) : T {
//     gen_assign_to_addr(&value)
//     return value
// }

func test_parameters() {

    test("taking address of parameters works", () => {
        var result = take_addr_of(0)
        return result == 25
    })
//    test("taking address of generic parameters works - 1", () => {
//        var result = gen_take_addr_of<short>(22)
//        return result == 2
//    })
//    test("taking address of generic parameters works - 2", () => {
//        var result = gen_take_addr_of<int>(22)
//        return result == 4
//    })
//    test("taking address of generic parameters works - 3", () => {
//        var result = gen_take_addr_of<bigint>(22)
//        return result == 8
//    })

}