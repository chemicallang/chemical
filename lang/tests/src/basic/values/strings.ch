func test_char_ptr_strings() {
    test("comptime string constant unused in other module works when imported", () => {
        return ext_mod_string_const[0] == 'H' && ext_mod_string_const[1] == 'e' && ext_mod_string_const[2] == 'l' && ext_mod_string_const[6] == 'W'
    })
    test("comptime string constant used in other module works when imported", () => {
        return used_mod_string_const[0] == 'H' && used_mod_string_const[1] == 'e' && used_mod_string_const[2] == 'l' && used_mod_string_const[6] == 'W'
    })
    test("check multiline strings work", () => {
        var first = """abcdefghij
klmnopqrstuvwxyz"""
        return strlen(first) == 28
    })
}