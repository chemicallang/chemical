// Whole-array assignment. Arrays are not assignable in C, so the C backend must
// copy the bytes (`memcpy`) rather than emit `dest = src` (invalid C). A missing
// memcpy produced invalid generated C for single- and multi-dimensional arrays,
// and for an array-typed struct field.
//
// Compiled-only: the interpreter does not support whole-array assignment.
// C-backend only: the LLVM backend's array assignment is a separate, still-broken
// path (it does not copy the elements), so guard this test to the C backend.

func test_array_assignment() {
    comptime if(intrinsics::get_backend_name() != "LLVM") {

        test("single-dimensional array assignment copies the elements", () => {
            var a : [4]int = [1, 2, 3, 4]
            var b : [4]int = []
            b = a
            b[0] = 9
            return a[0] == 1 && b[0] == 9 && b[1] == 2 && b[2] == 3 && b[3] == 4
        })

        test("multi-dimensional array assignment copies the elements", () => {
            var a : [2][2]int = []
            a[0][0] = 1
            a[0][1] = 2
            a[1][0] = 3
            a[1][1] = 4
            var b : [2][2]int = []
            b = a
            b[0][0] = 9
            return a[0][0] == 1 && b[0][0] == 9 && b[1][0] == 3 && b[1][1] == 4
        })

        test("array assignment copies from a struct field", () => {
            var holder = ArrayFieldHolder { bytes : [1u8, 2u8, 3u8, 4u8] }
            var other : [4]u8 = []
            other = holder.bytes
            other[0] = 9u8
            return holder.bytes[0] == 1u8 && other[0] == 9u8 && other[3] == 4u8
        })
    }
}

struct ArrayFieldHolder {
    var bytes : [4]u8
}

var large_array_drop_count = 0

struct LargeArrayDrop {
    var value : int
    @delete
    func delete(&mut self) {
        unsafe { large_array_drop_count = large_array_drop_count + 1 }
    }
}

struct LargeArrayStruct {
    var a : LargeArrayDrop
    var b : LargeArrayDrop
}

func test_large_array_partial_init() {
    test("large arrays track per-element drop flags", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayDrop
                arr[0] = LargeArrayDrop { value : 1 }
                arr[99] = LargeArrayDrop { value : 2 }
                if(arr[0].value != 1 || arr[99].value != 2) { return false }
            }
            return large_array_drop_count == 2
        }
    })
    test("large arrays of generated-glue structs track element leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayStruct
                arr[0] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 1 },
                    b : LargeArrayDrop { value : 2 }
                }
                arr[99] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 3 },
                    b : LargeArrayDrop { value : 4 }
                }
                if(arr[0].a.value != 1 || arr[99].b.value != 4) { return false }
            }
            return large_array_drop_count == 4
        }
    })
    test("large generated-glue arrays do not drop uninitialized element fields", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayStruct
                arr[0].a = LargeArrayDrop { value : 5 }
                if(arr[0].a.value != 5) { return false }
            }
            // Only the initialized leaf may be dropped; the sibling must not be.
            return large_array_drop_count <= 1
        }
    })
}

struct LargeArrayNested {
    var inner : [2]LargeArrayDrop
}

struct LargeArrayNestedStruct {
    var inner : [2]LargeArrayStruct
}

func test_large_array_nested_templates() {
    test("large generated-glue arrays track nested array leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNested
                arr[0] = LargeArrayNested {
                    inner : [LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }]
                }
                arr[99] = LargeArrayNested {
                    inner : [LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }]
                }
                if(arr[0].inner[1].value != 2 || arr[99].inner[0].value != 3) { return false }
            }
            return large_array_drop_count == 4
        }
    })
    test("large generated-glue arrays track nested struct-array leaves", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNestedStruct
                arr[0] = LargeArrayNestedStruct {
                    inner : [
                        LargeArrayStruct { a : LargeArrayDrop { value : 1 }, b : LargeArrayDrop { value : 2 } },
                        LargeArrayStruct { a : LargeArrayDrop { value : 3 }, b : LargeArrayDrop { value : 4 } }
                    ]
                }
                arr[99] = LargeArrayNestedStruct {
                    inner : [
                        LargeArrayStruct { a : LargeArrayDrop { value : 5 }, b : LargeArrayDrop { value : 6 } },
                        LargeArrayStruct { a : LargeArrayDrop { value : 7 }, b : LargeArrayDrop { value : 8 } }
                    ]
                }
            }
            return large_array_drop_count == 8
        }
    })
}


struct LargeArrayNestedLarge {
    var inner : [65]LargeArrayDrop
}

func test_large_array_nested_above_64() {
    test("large templates track inner arrays above 64 elements", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100]LargeArrayNestedLarge
                arr[0] = LargeArrayNestedLarge {
                    inner : [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                }
                if(arr[0].inner[64].value != 64) { return false }
            }
            return large_array_drop_count == 65
        }
    })
    test("small outer arrays destroy whole large direct arrays", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [2][65]LargeArrayDrop
                arr[0] = [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                if(arr[0][64].value != 64) { return false }
            }
            return large_array_drop_count == 65
        }
    })
    test("small outer arrays destroy whole large nested arrays", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [2]LargeArrayNestedLarge
                arr[0] = LargeArrayNestedLarge {
                    inner : [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }]
                }
                if(arr[0].inner[0].value != 0) { return false }
            }
            return large_array_drop_count == 65
        }
    })
}


func test_large_array_flat_inner() {
    test("large templates use a flat flag block for inner arrays above 1024", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var arr : [100][1025]LargeArrayDrop
                arr[0] = [LargeArrayDrop { value : 0 }, LargeArrayDrop { value : 1 }, LargeArrayDrop { value : 2 }, LargeArrayDrop { value : 3 }, LargeArrayDrop { value : 4 }, LargeArrayDrop { value : 5 }, LargeArrayDrop { value : 6 }, LargeArrayDrop { value : 7 }, LargeArrayDrop { value : 8 }, LargeArrayDrop { value : 9 }, LargeArrayDrop { value : 10 }, LargeArrayDrop { value : 11 }, LargeArrayDrop { value : 12 }, LargeArrayDrop { value : 13 }, LargeArrayDrop { value : 14 }, LargeArrayDrop { value : 15 }, LargeArrayDrop { value : 16 }, LargeArrayDrop { value : 17 }, LargeArrayDrop { value : 18 }, LargeArrayDrop { value : 19 }, LargeArrayDrop { value : 20 }, LargeArrayDrop { value : 21 }, LargeArrayDrop { value : 22 }, LargeArrayDrop { value : 23 }, LargeArrayDrop { value : 24 }, LargeArrayDrop { value : 25 }, LargeArrayDrop { value : 26 }, LargeArrayDrop { value : 27 }, LargeArrayDrop { value : 28 }, LargeArrayDrop { value : 29 }, LargeArrayDrop { value : 30 }, LargeArrayDrop { value : 31 }, LargeArrayDrop { value : 32 }, LargeArrayDrop { value : 33 }, LargeArrayDrop { value : 34 }, LargeArrayDrop { value : 35 }, LargeArrayDrop { value : 36 }, LargeArrayDrop { value : 37 }, LargeArrayDrop { value : 38 }, LargeArrayDrop { value : 39 }, LargeArrayDrop { value : 40 }, LargeArrayDrop { value : 41 }, LargeArrayDrop { value : 42 }, LargeArrayDrop { value : 43 }, LargeArrayDrop { value : 44 }, LargeArrayDrop { value : 45 }, LargeArrayDrop { value : 46 }, LargeArrayDrop { value : 47 }, LargeArrayDrop { value : 48 }, LargeArrayDrop { value : 49 }, LargeArrayDrop { value : 50 }, LargeArrayDrop { value : 51 }, LargeArrayDrop { value : 52 }, LargeArrayDrop { value : 53 }, LargeArrayDrop { value : 54 }, LargeArrayDrop { value : 55 }, LargeArrayDrop { value : 56 }, LargeArrayDrop { value : 57 }, LargeArrayDrop { value : 58 }, LargeArrayDrop { value : 59 }, LargeArrayDrop { value : 60 }, LargeArrayDrop { value : 61 }, LargeArrayDrop { value : 62 }, LargeArrayDrop { value : 63 }, LargeArrayDrop { value : 64 }, LargeArrayDrop { value : 65 }, LargeArrayDrop { value : 66 }, LargeArrayDrop { value : 67 }, LargeArrayDrop { value : 68 }, LargeArrayDrop { value : 69 }, LargeArrayDrop { value : 70 }, LargeArrayDrop { value : 71 }, LargeArrayDrop { value : 72 }, LargeArrayDrop { value : 73 }, LargeArrayDrop { value : 74 }, LargeArrayDrop { value : 75 }, LargeArrayDrop { value : 76 }, LargeArrayDrop { value : 77 }, LargeArrayDrop { value : 78 }, LargeArrayDrop { value : 79 }, LargeArrayDrop { value : 80 }, LargeArrayDrop { value : 81 }, LargeArrayDrop { value : 82 }, LargeArrayDrop { value : 83 }, LargeArrayDrop { value : 84 }, LargeArrayDrop { value : 85 }, LargeArrayDrop { value : 86 }, LargeArrayDrop { value : 87 }, LargeArrayDrop { value : 88 }, LargeArrayDrop { value : 89 }, LargeArrayDrop { value : 90 }, LargeArrayDrop { value : 91 }, LargeArrayDrop { value : 92 }, LargeArrayDrop { value : 93 }, LargeArrayDrop { value : 94 }, LargeArrayDrop { value : 95 }, LargeArrayDrop { value : 96 }, LargeArrayDrop { value : 97 }, LargeArrayDrop { value : 98 }, LargeArrayDrop { value : 99 }, LargeArrayDrop { value : 100 }, LargeArrayDrop { value : 101 }, LargeArrayDrop { value : 102 }, LargeArrayDrop { value : 103 }, LargeArrayDrop { value : 104 }, LargeArrayDrop { value : 105 }, LargeArrayDrop { value : 106 }, LargeArrayDrop { value : 107 }, LargeArrayDrop { value : 108 }, LargeArrayDrop { value : 109 }, LargeArrayDrop { value : 110 }, LargeArrayDrop { value : 111 }, LargeArrayDrop { value : 112 }, LargeArrayDrop { value : 113 }, LargeArrayDrop { value : 114 }, LargeArrayDrop { value : 115 }, LargeArrayDrop { value : 116 }, LargeArrayDrop { value : 117 }, LargeArrayDrop { value : 118 }, LargeArrayDrop { value : 119 }, LargeArrayDrop { value : 120 }, LargeArrayDrop { value : 121 }, LargeArrayDrop { value : 122 }, LargeArrayDrop { value : 123 }, LargeArrayDrop { value : 124 }, LargeArrayDrop { value : 125 }, LargeArrayDrop { value : 126 }, LargeArrayDrop { value : 127 }, LargeArrayDrop { value : 128 }, LargeArrayDrop { value : 129 }, LargeArrayDrop { value : 130 }, LargeArrayDrop { value : 131 }, LargeArrayDrop { value : 132 }, LargeArrayDrop { value : 133 }, LargeArrayDrop { value : 134 }, LargeArrayDrop { value : 135 }, LargeArrayDrop { value : 136 }, LargeArrayDrop { value : 137 }, LargeArrayDrop { value : 138 }, LargeArrayDrop { value : 139 }, LargeArrayDrop { value : 140 }, LargeArrayDrop { value : 141 }, LargeArrayDrop { value : 142 }, LargeArrayDrop { value : 143 }, LargeArrayDrop { value : 144 }, LargeArrayDrop { value : 145 }, LargeArrayDrop { value : 146 }, LargeArrayDrop { value : 147 }, LargeArrayDrop { value : 148 }, LargeArrayDrop { value : 149 }, LargeArrayDrop { value : 150 }, LargeArrayDrop { value : 151 }, LargeArrayDrop { value : 152 }, LargeArrayDrop { value : 153 }, LargeArrayDrop { value : 154 }, LargeArrayDrop { value : 155 }, LargeArrayDrop { value : 156 }, LargeArrayDrop { value : 157 }, LargeArrayDrop { value : 158 }, LargeArrayDrop { value : 159 }, LargeArrayDrop { value : 160 }, LargeArrayDrop { value : 161 }, LargeArrayDrop { value : 162 }, LargeArrayDrop { value : 163 }, LargeArrayDrop { value : 164 }, LargeArrayDrop { value : 165 }, LargeArrayDrop { value : 166 }, LargeArrayDrop { value : 167 }, LargeArrayDrop { value : 168 }, LargeArrayDrop { value : 169 }, LargeArrayDrop { value : 170 }, LargeArrayDrop { value : 171 }, LargeArrayDrop { value : 172 }, LargeArrayDrop { value : 173 }, LargeArrayDrop { value : 174 }, LargeArrayDrop { value : 175 }, LargeArrayDrop { value : 176 }, LargeArrayDrop { value : 177 }, LargeArrayDrop { value : 178 }, LargeArrayDrop { value : 179 }, LargeArrayDrop { value : 180 }, LargeArrayDrop { value : 181 }, LargeArrayDrop { value : 182 }, LargeArrayDrop { value : 183 }, LargeArrayDrop { value : 184 }, LargeArrayDrop { value : 185 }, LargeArrayDrop { value : 186 }, LargeArrayDrop { value : 187 }, LargeArrayDrop { value : 188 }, LargeArrayDrop { value : 189 }, LargeArrayDrop { value : 190 }, LargeArrayDrop { value : 191 }, LargeArrayDrop { value : 192 }, LargeArrayDrop { value : 193 }, LargeArrayDrop { value : 194 }, LargeArrayDrop { value : 195 }, LargeArrayDrop { value : 196 }, LargeArrayDrop { value : 197 }, LargeArrayDrop { value : 198 }, LargeArrayDrop { value : 199 }, LargeArrayDrop { value : 200 }, LargeArrayDrop { value : 201 }, LargeArrayDrop { value : 202 }, LargeArrayDrop { value : 203 }, LargeArrayDrop { value : 204 }, LargeArrayDrop { value : 205 }, LargeArrayDrop { value : 206 }, LargeArrayDrop { value : 207 }, LargeArrayDrop { value : 208 }, LargeArrayDrop { value : 209 }, LargeArrayDrop { value : 210 }, LargeArrayDrop { value : 211 }, LargeArrayDrop { value : 212 }, LargeArrayDrop { value : 213 }, LargeArrayDrop { value : 214 }, LargeArrayDrop { value : 215 }, LargeArrayDrop { value : 216 }, LargeArrayDrop { value : 217 }, LargeArrayDrop { value : 218 }, LargeArrayDrop { value : 219 }, LargeArrayDrop { value : 220 }, LargeArrayDrop { value : 221 }, LargeArrayDrop { value : 222 }, LargeArrayDrop { value : 223 }, LargeArrayDrop { value : 224 }, LargeArrayDrop { value : 225 }, LargeArrayDrop { value : 226 }, LargeArrayDrop { value : 227 }, LargeArrayDrop { value : 228 }, LargeArrayDrop { value : 229 }, LargeArrayDrop { value : 230 }, LargeArrayDrop { value : 231 }, LargeArrayDrop { value : 232 }, LargeArrayDrop { value : 233 }, LargeArrayDrop { value : 234 }, LargeArrayDrop { value : 235 }, LargeArrayDrop { value : 236 }, LargeArrayDrop { value : 237 }, LargeArrayDrop { value : 238 }, LargeArrayDrop { value : 239 }, LargeArrayDrop { value : 240 }, LargeArrayDrop { value : 241 }, LargeArrayDrop { value : 242 }, LargeArrayDrop { value : 243 }, LargeArrayDrop { value : 244 }, LargeArrayDrop { value : 245 }, LargeArrayDrop { value : 246 }, LargeArrayDrop { value : 247 }, LargeArrayDrop { value : 248 }, LargeArrayDrop { value : 249 }, LargeArrayDrop { value : 250 }, LargeArrayDrop { value : 251 }, LargeArrayDrop { value : 252 }, LargeArrayDrop { value : 253 }, LargeArrayDrop { value : 254 }, LargeArrayDrop { value : 255 }, LargeArrayDrop { value : 256 }, LargeArrayDrop { value : 257 }, LargeArrayDrop { value : 258 }, LargeArrayDrop { value : 259 }, LargeArrayDrop { value : 260 }, LargeArrayDrop { value : 261 }, LargeArrayDrop { value : 262 }, LargeArrayDrop { value : 263 }, LargeArrayDrop { value : 264 }, LargeArrayDrop { value : 265 }, LargeArrayDrop { value : 266 }, LargeArrayDrop { value : 267 }, LargeArrayDrop { value : 268 }, LargeArrayDrop { value : 269 }, LargeArrayDrop { value : 270 }, LargeArrayDrop { value : 271 }, LargeArrayDrop { value : 272 }, LargeArrayDrop { value : 273 }, LargeArrayDrop { value : 274 }, LargeArrayDrop { value : 275 }, LargeArrayDrop { value : 276 }, LargeArrayDrop { value : 277 }, LargeArrayDrop { value : 278 }, LargeArrayDrop { value : 279 }, LargeArrayDrop { value : 280 }, LargeArrayDrop { value : 281 }, LargeArrayDrop { value : 282 }, LargeArrayDrop { value : 283 }, LargeArrayDrop { value : 284 }, LargeArrayDrop { value : 285 }, LargeArrayDrop { value : 286 }, LargeArrayDrop { value : 287 }, LargeArrayDrop { value : 288 }, LargeArrayDrop { value : 289 }, LargeArrayDrop { value : 290 }, LargeArrayDrop { value : 291 }, LargeArrayDrop { value : 292 }, LargeArrayDrop { value : 293 }, LargeArrayDrop { value : 294 }, LargeArrayDrop { value : 295 }, LargeArrayDrop { value : 296 }, LargeArrayDrop { value : 297 }, LargeArrayDrop { value : 298 }, LargeArrayDrop { value : 299 }, LargeArrayDrop { value : 300 }, LargeArrayDrop { value : 301 }, LargeArrayDrop { value : 302 }, LargeArrayDrop { value : 303 }, LargeArrayDrop { value : 304 }, LargeArrayDrop { value : 305 }, LargeArrayDrop { value : 306 }, LargeArrayDrop { value : 307 }, LargeArrayDrop { value : 308 }, LargeArrayDrop { value : 309 }, LargeArrayDrop { value : 310 }, LargeArrayDrop { value : 311 }, LargeArrayDrop { value : 312 }, LargeArrayDrop { value : 313 }, LargeArrayDrop { value : 314 }, LargeArrayDrop { value : 315 }, LargeArrayDrop { value : 316 }, LargeArrayDrop { value : 317 }, LargeArrayDrop { value : 318 }, LargeArrayDrop { value : 319 }, LargeArrayDrop { value : 320 }, LargeArrayDrop { value : 321 }, LargeArrayDrop { value : 322 }, LargeArrayDrop { value : 323 }, LargeArrayDrop { value : 324 }, LargeArrayDrop { value : 325 }, LargeArrayDrop { value : 326 }, LargeArrayDrop { value : 327 }, LargeArrayDrop { value : 328 }, LargeArrayDrop { value : 329 }, LargeArrayDrop { value : 330 }, LargeArrayDrop { value : 331 }, LargeArrayDrop { value : 332 }, LargeArrayDrop { value : 333 }, LargeArrayDrop { value : 334 }, LargeArrayDrop { value : 335 }, LargeArrayDrop { value : 336 }, LargeArrayDrop { value : 337 }, LargeArrayDrop { value : 338 }, LargeArrayDrop { value : 339 }, LargeArrayDrop { value : 340 }, LargeArrayDrop { value : 341 }, LargeArrayDrop { value : 342 }, LargeArrayDrop { value : 343 }, LargeArrayDrop { value : 344 }, LargeArrayDrop { value : 345 }, LargeArrayDrop { value : 346 }, LargeArrayDrop { value : 347 }, LargeArrayDrop { value : 348 }, LargeArrayDrop { value : 349 }, LargeArrayDrop { value : 350 }, LargeArrayDrop { value : 351 }, LargeArrayDrop { value : 352 }, LargeArrayDrop { value : 353 }, LargeArrayDrop { value : 354 }, LargeArrayDrop { value : 355 }, LargeArrayDrop { value : 356 }, LargeArrayDrop { value : 357 }, LargeArrayDrop { value : 358 }, LargeArrayDrop { value : 359 }, LargeArrayDrop { value : 360 }, LargeArrayDrop { value : 361 }, LargeArrayDrop { value : 362 }, LargeArrayDrop { value : 363 }, LargeArrayDrop { value : 364 }, LargeArrayDrop { value : 365 }, LargeArrayDrop { value : 366 }, LargeArrayDrop { value : 367 }, LargeArrayDrop { value : 368 }, LargeArrayDrop { value : 369 }, LargeArrayDrop { value : 370 }, LargeArrayDrop { value : 371 }, LargeArrayDrop { value : 372 }, LargeArrayDrop { value : 373 }, LargeArrayDrop { value : 374 }, LargeArrayDrop { value : 375 }, LargeArrayDrop { value : 376 }, LargeArrayDrop { value : 377 }, LargeArrayDrop { value : 378 }, LargeArrayDrop { value : 379 }, LargeArrayDrop { value : 380 }, LargeArrayDrop { value : 381 }, LargeArrayDrop { value : 382 }, LargeArrayDrop { value : 383 }, LargeArrayDrop { value : 384 }, LargeArrayDrop { value : 385 }, LargeArrayDrop { value : 386 }, LargeArrayDrop { value : 387 }, LargeArrayDrop { value : 388 }, LargeArrayDrop { value : 389 }, LargeArrayDrop { value : 390 }, LargeArrayDrop { value : 391 }, LargeArrayDrop { value : 392 }, LargeArrayDrop { value : 393 }, LargeArrayDrop { value : 394 }, LargeArrayDrop { value : 395 }, LargeArrayDrop { value : 396 }, LargeArrayDrop { value : 397 }, LargeArrayDrop { value : 398 }, LargeArrayDrop { value : 399 }, LargeArrayDrop { value : 400 }, LargeArrayDrop { value : 401 }, LargeArrayDrop { value : 402 }, LargeArrayDrop { value : 403 }, LargeArrayDrop { value : 404 }, LargeArrayDrop { value : 405 }, LargeArrayDrop { value : 406 }, LargeArrayDrop { value : 407 }, LargeArrayDrop { value : 408 }, LargeArrayDrop { value : 409 }, LargeArrayDrop { value : 410 }, LargeArrayDrop { value : 411 }, LargeArrayDrop { value : 412 }, LargeArrayDrop { value : 413 }, LargeArrayDrop { value : 414 }, LargeArrayDrop { value : 415 }, LargeArrayDrop { value : 416 }, LargeArrayDrop { value : 417 }, LargeArrayDrop { value : 418 }, LargeArrayDrop { value : 419 }, LargeArrayDrop { value : 420 }, LargeArrayDrop { value : 421 }, LargeArrayDrop { value : 422 }, LargeArrayDrop { value : 423 }, LargeArrayDrop { value : 424 }, LargeArrayDrop { value : 425 }, LargeArrayDrop { value : 426 }, LargeArrayDrop { value : 427 }, LargeArrayDrop { value : 428 }, LargeArrayDrop { value : 429 }, LargeArrayDrop { value : 430 }, LargeArrayDrop { value : 431 }, LargeArrayDrop { value : 432 }, LargeArrayDrop { value : 433 }, LargeArrayDrop { value : 434 }, LargeArrayDrop { value : 435 }, LargeArrayDrop { value : 436 }, LargeArrayDrop { value : 437 }, LargeArrayDrop { value : 438 }, LargeArrayDrop { value : 439 }, LargeArrayDrop { value : 440 }, LargeArrayDrop { value : 441 }, LargeArrayDrop { value : 442 }, LargeArrayDrop { value : 443 }, LargeArrayDrop { value : 444 }, LargeArrayDrop { value : 445 }, LargeArrayDrop { value : 446 }, LargeArrayDrop { value : 447 }, LargeArrayDrop { value : 448 }, LargeArrayDrop { value : 449 }, LargeArrayDrop { value : 450 }, LargeArrayDrop { value : 451 }, LargeArrayDrop { value : 452 }, LargeArrayDrop { value : 453 }, LargeArrayDrop { value : 454 }, LargeArrayDrop { value : 455 }, LargeArrayDrop { value : 456 }, LargeArrayDrop { value : 457 }, LargeArrayDrop { value : 458 }, LargeArrayDrop { value : 459 }, LargeArrayDrop { value : 460 }, LargeArrayDrop { value : 461 }, LargeArrayDrop { value : 462 }, LargeArrayDrop { value : 463 }, LargeArrayDrop { value : 464 }, LargeArrayDrop { value : 465 }, LargeArrayDrop { value : 466 }, LargeArrayDrop { value : 467 }, LargeArrayDrop { value : 468 }, LargeArrayDrop { value : 469 }, LargeArrayDrop { value : 470 }, LargeArrayDrop { value : 471 }, LargeArrayDrop { value : 472 }, LargeArrayDrop { value : 473 }, LargeArrayDrop { value : 474 }, LargeArrayDrop { value : 475 }, LargeArrayDrop { value : 476 }, LargeArrayDrop { value : 477 }, LargeArrayDrop { value : 478 }, LargeArrayDrop { value : 479 }, LargeArrayDrop { value : 480 }, LargeArrayDrop { value : 481 }, LargeArrayDrop { value : 482 }, LargeArrayDrop { value : 483 }, LargeArrayDrop { value : 484 }, LargeArrayDrop { value : 485 }, LargeArrayDrop { value : 486 }, LargeArrayDrop { value : 487 }, LargeArrayDrop { value : 488 }, LargeArrayDrop { value : 489 }, LargeArrayDrop { value : 490 }, LargeArrayDrop { value : 491 }, LargeArrayDrop { value : 492 }, LargeArrayDrop { value : 493 }, LargeArrayDrop { value : 494 }, LargeArrayDrop { value : 495 }, LargeArrayDrop { value : 496 }, LargeArrayDrop { value : 497 }, LargeArrayDrop { value : 498 }, LargeArrayDrop { value : 499 }, LargeArrayDrop { value : 500 }, LargeArrayDrop { value : 501 }, LargeArrayDrop { value : 502 }, LargeArrayDrop { value : 503 }, LargeArrayDrop { value : 504 }, LargeArrayDrop { value : 505 }, LargeArrayDrop { value : 506 }, LargeArrayDrop { value : 507 }, LargeArrayDrop { value : 508 }, LargeArrayDrop { value : 509 }, LargeArrayDrop { value : 510 }, LargeArrayDrop { value : 511 }, LargeArrayDrop { value : 512 }, LargeArrayDrop { value : 513 }, LargeArrayDrop { value : 514 }, LargeArrayDrop { value : 515 }, LargeArrayDrop { value : 516 }, LargeArrayDrop { value : 517 }, LargeArrayDrop { value : 518 }, LargeArrayDrop { value : 519 }, LargeArrayDrop { value : 520 }, LargeArrayDrop { value : 521 }, LargeArrayDrop { value : 522 }, LargeArrayDrop { value : 523 }, LargeArrayDrop { value : 524 }, LargeArrayDrop { value : 525 }, LargeArrayDrop { value : 526 }, LargeArrayDrop { value : 527 }, LargeArrayDrop { value : 528 }, LargeArrayDrop { value : 529 }, LargeArrayDrop { value : 530 }, LargeArrayDrop { value : 531 }, LargeArrayDrop { value : 532 }, LargeArrayDrop { value : 533 }, LargeArrayDrop { value : 534 }, LargeArrayDrop { value : 535 }, LargeArrayDrop { value : 536 }, LargeArrayDrop { value : 537 }, LargeArrayDrop { value : 538 }, LargeArrayDrop { value : 539 }, LargeArrayDrop { value : 540 }, LargeArrayDrop { value : 541 }, LargeArrayDrop { value : 542 }, LargeArrayDrop { value : 543 }, LargeArrayDrop { value : 544 }, LargeArrayDrop { value : 545 }, LargeArrayDrop { value : 546 }, LargeArrayDrop { value : 547 }, LargeArrayDrop { value : 548 }, LargeArrayDrop { value : 549 }, LargeArrayDrop { value : 550 }, LargeArrayDrop { value : 551 }, LargeArrayDrop { value : 552 }, LargeArrayDrop { value : 553 }, LargeArrayDrop { value : 554 }, LargeArrayDrop { value : 555 }, LargeArrayDrop { value : 556 }, LargeArrayDrop { value : 557 }, LargeArrayDrop { value : 558 }, LargeArrayDrop { value : 559 }, LargeArrayDrop { value : 560 }, LargeArrayDrop { value : 561 }, LargeArrayDrop { value : 562 }, LargeArrayDrop { value : 563 }, LargeArrayDrop { value : 564 }, LargeArrayDrop { value : 565 }, LargeArrayDrop { value : 566 }, LargeArrayDrop { value : 567 }, LargeArrayDrop { value : 568 }, LargeArrayDrop { value : 569 }, LargeArrayDrop { value : 570 }, LargeArrayDrop { value : 571 }, LargeArrayDrop { value : 572 }, LargeArrayDrop { value : 573 }, LargeArrayDrop { value : 574 }, LargeArrayDrop { value : 575 }, LargeArrayDrop { value : 576 }, LargeArrayDrop { value : 577 }, LargeArrayDrop { value : 578 }, LargeArrayDrop { value : 579 }, LargeArrayDrop { value : 580 }, LargeArrayDrop { value : 581 }, LargeArrayDrop { value : 582 }, LargeArrayDrop { value : 583 }, LargeArrayDrop { value : 584 }, LargeArrayDrop { value : 585 }, LargeArrayDrop { value : 586 }, LargeArrayDrop { value : 587 }, LargeArrayDrop { value : 588 }, LargeArrayDrop { value : 589 }, LargeArrayDrop { value : 590 }, LargeArrayDrop { value : 591 }, LargeArrayDrop { value : 592 }, LargeArrayDrop { value : 593 }, LargeArrayDrop { value : 594 }, LargeArrayDrop { value : 595 }, LargeArrayDrop { value : 596 }, LargeArrayDrop { value : 597 }, LargeArrayDrop { value : 598 }, LargeArrayDrop { value : 599 }, LargeArrayDrop { value : 600 }, LargeArrayDrop { value : 601 }, LargeArrayDrop { value : 602 }, LargeArrayDrop { value : 603 }, LargeArrayDrop { value : 604 }, LargeArrayDrop { value : 605 }, LargeArrayDrop { value : 606 }, LargeArrayDrop { value : 607 }, LargeArrayDrop { value : 608 }, LargeArrayDrop { value : 609 }, LargeArrayDrop { value : 610 }, LargeArrayDrop { value : 611 }, LargeArrayDrop { value : 612 }, LargeArrayDrop { value : 613 }, LargeArrayDrop { value : 614 }, LargeArrayDrop { value : 615 }, LargeArrayDrop { value : 616 }, LargeArrayDrop { value : 617 }, LargeArrayDrop { value : 618 }, LargeArrayDrop { value : 619 }, LargeArrayDrop { value : 620 }, LargeArrayDrop { value : 621 }, LargeArrayDrop { value : 622 }, LargeArrayDrop { value : 623 }, LargeArrayDrop { value : 624 }, LargeArrayDrop { value : 625 }, LargeArrayDrop { value : 626 }, LargeArrayDrop { value : 627 }, LargeArrayDrop { value : 628 }, LargeArrayDrop { value : 629 }, LargeArrayDrop { value : 630 }, LargeArrayDrop { value : 631 }, LargeArrayDrop { value : 632 }, LargeArrayDrop { value : 633 }, LargeArrayDrop { value : 634 }, LargeArrayDrop { value : 635 }, LargeArrayDrop { value : 636 }, LargeArrayDrop { value : 637 }, LargeArrayDrop { value : 638 }, LargeArrayDrop { value : 639 }, LargeArrayDrop { value : 640 }, LargeArrayDrop { value : 641 }, LargeArrayDrop { value : 642 }, LargeArrayDrop { value : 643 }, LargeArrayDrop { value : 644 }, LargeArrayDrop { value : 645 }, LargeArrayDrop { value : 646 }, LargeArrayDrop { value : 647 }, LargeArrayDrop { value : 648 }, LargeArrayDrop { value : 649 }, LargeArrayDrop { value : 650 }, LargeArrayDrop { value : 651 }, LargeArrayDrop { value : 652 }, LargeArrayDrop { value : 653 }, LargeArrayDrop { value : 654 }, LargeArrayDrop { value : 655 }, LargeArrayDrop { value : 656 }, LargeArrayDrop { value : 657 }, LargeArrayDrop { value : 658 }, LargeArrayDrop { value : 659 }, LargeArrayDrop { value : 660 }, LargeArrayDrop { value : 661 }, LargeArrayDrop { value : 662 }, LargeArrayDrop { value : 663 }, LargeArrayDrop { value : 664 }, LargeArrayDrop { value : 665 }, LargeArrayDrop { value : 666 }, LargeArrayDrop { value : 667 }, LargeArrayDrop { value : 668 }, LargeArrayDrop { value : 669 }, LargeArrayDrop { value : 670 }, LargeArrayDrop { value : 671 }, LargeArrayDrop { value : 672 }, LargeArrayDrop { value : 673 }, LargeArrayDrop { value : 674 }, LargeArrayDrop { value : 675 }, LargeArrayDrop { value : 676 }, LargeArrayDrop { value : 677 }, LargeArrayDrop { value : 678 }, LargeArrayDrop { value : 679 }, LargeArrayDrop { value : 680 }, LargeArrayDrop { value : 681 }, LargeArrayDrop { value : 682 }, LargeArrayDrop { value : 683 }, LargeArrayDrop { value : 684 }, LargeArrayDrop { value : 685 }, LargeArrayDrop { value : 686 }, LargeArrayDrop { value : 687 }, LargeArrayDrop { value : 688 }, LargeArrayDrop { value : 689 }, LargeArrayDrop { value : 690 }, LargeArrayDrop { value : 691 }, LargeArrayDrop { value : 692 }, LargeArrayDrop { value : 693 }, LargeArrayDrop { value : 694 }, LargeArrayDrop { value : 695 }, LargeArrayDrop { value : 696 }, LargeArrayDrop { value : 697 }, LargeArrayDrop { value : 698 }, LargeArrayDrop { value : 699 }, LargeArrayDrop { value : 700 }, LargeArrayDrop { value : 701 }, LargeArrayDrop { value : 702 }, LargeArrayDrop { value : 703 }, LargeArrayDrop { value : 704 }, LargeArrayDrop { value : 705 }, LargeArrayDrop { value : 706 }, LargeArrayDrop { value : 707 }, LargeArrayDrop { value : 708 }, LargeArrayDrop { value : 709 }, LargeArrayDrop { value : 710 }, LargeArrayDrop { value : 711 }, LargeArrayDrop { value : 712 }, LargeArrayDrop { value : 713 }, LargeArrayDrop { value : 714 }, LargeArrayDrop { value : 715 }, LargeArrayDrop { value : 716 }, LargeArrayDrop { value : 717 }, LargeArrayDrop { value : 718 }, LargeArrayDrop { value : 719 }, LargeArrayDrop { value : 720 }, LargeArrayDrop { value : 721 }, LargeArrayDrop { value : 722 }, LargeArrayDrop { value : 723 }, LargeArrayDrop { value : 724 }, LargeArrayDrop { value : 725 }, LargeArrayDrop { value : 726 }, LargeArrayDrop { value : 727 }, LargeArrayDrop { value : 728 }, LargeArrayDrop { value : 729 }, LargeArrayDrop { value : 730 }, LargeArrayDrop { value : 731 }, LargeArrayDrop { value : 732 }, LargeArrayDrop { value : 733 }, LargeArrayDrop { value : 734 }, LargeArrayDrop { value : 735 }, LargeArrayDrop { value : 736 }, LargeArrayDrop { value : 737 }, LargeArrayDrop { value : 738 }, LargeArrayDrop { value : 739 }, LargeArrayDrop { value : 740 }, LargeArrayDrop { value : 741 }, LargeArrayDrop { value : 742 }, LargeArrayDrop { value : 743 }, LargeArrayDrop { value : 744 }, LargeArrayDrop { value : 745 }, LargeArrayDrop { value : 746 }, LargeArrayDrop { value : 747 }, LargeArrayDrop { value : 748 }, LargeArrayDrop { value : 749 }, LargeArrayDrop { value : 750 }, LargeArrayDrop { value : 751 }, LargeArrayDrop { value : 752 }, LargeArrayDrop { value : 753 }, LargeArrayDrop { value : 754 }, LargeArrayDrop { value : 755 }, LargeArrayDrop { value : 756 }, LargeArrayDrop { value : 757 }, LargeArrayDrop { value : 758 }, LargeArrayDrop { value : 759 }, LargeArrayDrop { value : 760 }, LargeArrayDrop { value : 761 }, LargeArrayDrop { value : 762 }, LargeArrayDrop { value : 763 }, LargeArrayDrop { value : 764 }, LargeArrayDrop { value : 765 }, LargeArrayDrop { value : 766 }, LargeArrayDrop { value : 767 }, LargeArrayDrop { value : 768 }, LargeArrayDrop { value : 769 }, LargeArrayDrop { value : 770 }, LargeArrayDrop { value : 771 }, LargeArrayDrop { value : 772 }, LargeArrayDrop { value : 773 }, LargeArrayDrop { value : 774 }, LargeArrayDrop { value : 775 }, LargeArrayDrop { value : 776 }, LargeArrayDrop { value : 777 }, LargeArrayDrop { value : 778 }, LargeArrayDrop { value : 779 }, LargeArrayDrop { value : 780 }, LargeArrayDrop { value : 781 }, LargeArrayDrop { value : 782 }, LargeArrayDrop { value : 783 }, LargeArrayDrop { value : 784 }, LargeArrayDrop { value : 785 }, LargeArrayDrop { value : 786 }, LargeArrayDrop { value : 787 }, LargeArrayDrop { value : 788 }, LargeArrayDrop { value : 789 }, LargeArrayDrop { value : 790 }, LargeArrayDrop { value : 791 }, LargeArrayDrop { value : 792 }, LargeArrayDrop { value : 793 }, LargeArrayDrop { value : 794 }, LargeArrayDrop { value : 795 }, LargeArrayDrop { value : 796 }, LargeArrayDrop { value : 797 }, LargeArrayDrop { value : 798 }, LargeArrayDrop { value : 799 }, LargeArrayDrop { value : 800 }, LargeArrayDrop { value : 801 }, LargeArrayDrop { value : 802 }, LargeArrayDrop { value : 803 }, LargeArrayDrop { value : 804 }, LargeArrayDrop { value : 805 }, LargeArrayDrop { value : 806 }, LargeArrayDrop { value : 807 }, LargeArrayDrop { value : 808 }, LargeArrayDrop { value : 809 }, LargeArrayDrop { value : 810 }, LargeArrayDrop { value : 811 }, LargeArrayDrop { value : 812 }, LargeArrayDrop { value : 813 }, LargeArrayDrop { value : 814 }, LargeArrayDrop { value : 815 }, LargeArrayDrop { value : 816 }, LargeArrayDrop { value : 817 }, LargeArrayDrop { value : 818 }, LargeArrayDrop { value : 819 }, LargeArrayDrop { value : 820 }, LargeArrayDrop { value : 821 }, LargeArrayDrop { value : 822 }, LargeArrayDrop { value : 823 }, LargeArrayDrop { value : 824 }, LargeArrayDrop { value : 825 }, LargeArrayDrop { value : 826 }, LargeArrayDrop { value : 827 }, LargeArrayDrop { value : 828 }, LargeArrayDrop { value : 829 }, LargeArrayDrop { value : 830 }, LargeArrayDrop { value : 831 }, LargeArrayDrop { value : 832 }, LargeArrayDrop { value : 833 }, LargeArrayDrop { value : 834 }, LargeArrayDrop { value : 835 }, LargeArrayDrop { value : 836 }, LargeArrayDrop { value : 837 }, LargeArrayDrop { value : 838 }, LargeArrayDrop { value : 839 }, LargeArrayDrop { value : 840 }, LargeArrayDrop { value : 841 }, LargeArrayDrop { value : 842 }, LargeArrayDrop { value : 843 }, LargeArrayDrop { value : 844 }, LargeArrayDrop { value : 845 }, LargeArrayDrop { value : 846 }, LargeArrayDrop { value : 847 }, LargeArrayDrop { value : 848 }, LargeArrayDrop { value : 849 }, LargeArrayDrop { value : 850 }, LargeArrayDrop { value : 851 }, LargeArrayDrop { value : 852 }, LargeArrayDrop { value : 853 }, LargeArrayDrop { value : 854 }, LargeArrayDrop { value : 855 }, LargeArrayDrop { value : 856 }, LargeArrayDrop { value : 857 }, LargeArrayDrop { value : 858 }, LargeArrayDrop { value : 859 }, LargeArrayDrop { value : 860 }, LargeArrayDrop { value : 861 }, LargeArrayDrop { value : 862 }, LargeArrayDrop { value : 863 }, LargeArrayDrop { value : 864 }, LargeArrayDrop { value : 865 }, LargeArrayDrop { value : 866 }, LargeArrayDrop { value : 867 }, LargeArrayDrop { value : 868 }, LargeArrayDrop { value : 869 }, LargeArrayDrop { value : 870 }, LargeArrayDrop { value : 871 }, LargeArrayDrop { value : 872 }, LargeArrayDrop { value : 873 }, LargeArrayDrop { value : 874 }, LargeArrayDrop { value : 875 }, LargeArrayDrop { value : 876 }, LargeArrayDrop { value : 877 }, LargeArrayDrop { value : 878 }, LargeArrayDrop { value : 879 }, LargeArrayDrop { value : 880 }, LargeArrayDrop { value : 881 }, LargeArrayDrop { value : 882 }, LargeArrayDrop { value : 883 }, LargeArrayDrop { value : 884 }, LargeArrayDrop { value : 885 }, LargeArrayDrop { value : 886 }, LargeArrayDrop { value : 887 }, LargeArrayDrop { value : 888 }, LargeArrayDrop { value : 889 }, LargeArrayDrop { value : 890 }, LargeArrayDrop { value : 891 }, LargeArrayDrop { value : 892 }, LargeArrayDrop { value : 893 }, LargeArrayDrop { value : 894 }, LargeArrayDrop { value : 895 }, LargeArrayDrop { value : 896 }, LargeArrayDrop { value : 897 }, LargeArrayDrop { value : 898 }, LargeArrayDrop { value : 899 }, LargeArrayDrop { value : 900 }, LargeArrayDrop { value : 901 }, LargeArrayDrop { value : 902 }, LargeArrayDrop { value : 903 }, LargeArrayDrop { value : 904 }, LargeArrayDrop { value : 905 }, LargeArrayDrop { value : 906 }, LargeArrayDrop { value : 907 }, LargeArrayDrop { value : 908 }, LargeArrayDrop { value : 909 }, LargeArrayDrop { value : 910 }, LargeArrayDrop { value : 911 }, LargeArrayDrop { value : 912 }, LargeArrayDrop { value : 913 }, LargeArrayDrop { value : 914 }, LargeArrayDrop { value : 915 }, LargeArrayDrop { value : 916 }, LargeArrayDrop { value : 917 }, LargeArrayDrop { value : 918 }, LargeArrayDrop { value : 919 }, LargeArrayDrop { value : 920 }, LargeArrayDrop { value : 921 }, LargeArrayDrop { value : 922 }, LargeArrayDrop { value : 923 }, LargeArrayDrop { value : 924 }, LargeArrayDrop { value : 925 }, LargeArrayDrop { value : 926 }, LargeArrayDrop { value : 927 }, LargeArrayDrop { value : 928 }, LargeArrayDrop { value : 929 }, LargeArrayDrop { value : 930 }, LargeArrayDrop { value : 931 }, LargeArrayDrop { value : 932 }, LargeArrayDrop { value : 933 }, LargeArrayDrop { value : 934 }, LargeArrayDrop { value : 935 }, LargeArrayDrop { value : 936 }, LargeArrayDrop { value : 937 }, LargeArrayDrop { value : 938 }, LargeArrayDrop { value : 939 }, LargeArrayDrop { value : 940 }, LargeArrayDrop { value : 941 }, LargeArrayDrop { value : 942 }, LargeArrayDrop { value : 943 }, LargeArrayDrop { value : 944 }, LargeArrayDrop { value : 945 }, LargeArrayDrop { value : 946 }, LargeArrayDrop { value : 947 }, LargeArrayDrop { value : 948 }, LargeArrayDrop { value : 949 }, LargeArrayDrop { value : 950 }, LargeArrayDrop { value : 951 }, LargeArrayDrop { value : 952 }, LargeArrayDrop { value : 953 }, LargeArrayDrop { value : 954 }, LargeArrayDrop { value : 955 }, LargeArrayDrop { value : 956 }, LargeArrayDrop { value : 957 }, LargeArrayDrop { value : 958 }, LargeArrayDrop { value : 959 }, LargeArrayDrop { value : 960 }, LargeArrayDrop { value : 961 }, LargeArrayDrop { value : 962 }, LargeArrayDrop { value : 963 }, LargeArrayDrop { value : 964 }, LargeArrayDrop { value : 965 }, LargeArrayDrop { value : 966 }, LargeArrayDrop { value : 967 }, LargeArrayDrop { value : 968 }, LargeArrayDrop { value : 969 }, LargeArrayDrop { value : 970 }, LargeArrayDrop { value : 971 }, LargeArrayDrop { value : 972 }, LargeArrayDrop { value : 973 }, LargeArrayDrop { value : 974 }, LargeArrayDrop { value : 975 }, LargeArrayDrop { value : 976 }, LargeArrayDrop { value : 977 }, LargeArrayDrop { value : 978 }, LargeArrayDrop { value : 979 }, LargeArrayDrop { value : 980 }, LargeArrayDrop { value : 981 }, LargeArrayDrop { value : 982 }, LargeArrayDrop { value : 983 }, LargeArrayDrop { value : 984 }, LargeArrayDrop { value : 985 }, LargeArrayDrop { value : 986 }, LargeArrayDrop { value : 987 }, LargeArrayDrop { value : 988 }, LargeArrayDrop { value : 989 }, LargeArrayDrop { value : 990 }, LargeArrayDrop { value : 991 }, LargeArrayDrop { value : 992 }, LargeArrayDrop { value : 993 }, LargeArrayDrop { value : 994 }, LargeArrayDrop { value : 995 }, LargeArrayDrop { value : 996 }, LargeArrayDrop { value : 997 }, LargeArrayDrop { value : 998 }, LargeArrayDrop { value : 999 }, LargeArrayDrop { value : 1000 }, LargeArrayDrop { value : 1001 }, LargeArrayDrop { value : 1002 }, LargeArrayDrop { value : 1003 }, LargeArrayDrop { value : 1004 }, LargeArrayDrop { value : 1005 }, LargeArrayDrop { value : 1006 }, LargeArrayDrop { value : 1007 }, LargeArrayDrop { value : 1008 }, LargeArrayDrop { value : 1009 }, LargeArrayDrop { value : 1010 }, LargeArrayDrop { value : 1011 }, LargeArrayDrop { value : 1012 }, LargeArrayDrop { value : 1013 }, LargeArrayDrop { value : 1014 }, LargeArrayDrop { value : 1015 }, LargeArrayDrop { value : 1016 }, LargeArrayDrop { value : 1017 }, LargeArrayDrop { value : 1018 }, LargeArrayDrop { value : 1019 }, LargeArrayDrop { value : 1020 }, LargeArrayDrop { value : 1021 }, LargeArrayDrop { value : 1022 }, LargeArrayDrop { value : 1023 }, LargeArrayDrop { value : 1024 }]
                if(arr[0][1024].value != 1024) { return false }
            }
            return large_array_drop_count == 1025
        }
    })
}

struct LargeArrayFieldHolder {
    var items : [100]LargeArrayStruct
}

struct SmallArrayFieldHolder {
    var items : [2]LargeArrayStruct
}

func test_array_field_element_assignment() {
    test("large generated-glue array fields track element assignment", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var h : LargeArrayFieldHolder
                h.items[0] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 1 },
                    b : LargeArrayDrop { value : 2 }
                }
                h.items[99] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 3 },
                    b : LargeArrayDrop { value : 4 }
                }
                if(h.items[0].b.value != 2 || h.items[99].a.value != 3) { return false }
            }
            return large_array_drop_count == 4
        }
    })
    test("small array fields track element assignment", () => {
        unsafe {
            large_array_drop_count = 0
            {
                var h : SmallArrayFieldHolder
                h.items[0] = LargeArrayStruct {
                    a : LargeArrayDrop { value : 5 },
                    b : LargeArrayDrop { value : 6 }
                }
                if(h.items[0].a.value != 5) { return false }
            }
            return large_array_drop_count == 2
        }
    })
}
