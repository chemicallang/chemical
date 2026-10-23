// Copyright (c) Chemical Language Foundation 2026.

/**
 * `PhantomData` is the zero-sized marker used to tie generic parameters and
 * lifetimes to a type without storing a value. These tests cover the marker
 * itself and its use as a field type in a lifetime-generic struct.
 */
struct PhantomHolder<'a, T> {
    var marker : std::PhantomData<&'a T>
    var value : int
}

func test_phantom_data() {
    test("PhantomData is zero sized", () => {
        return sizeof(std::PhantomData<&int>) == 0
    })
    test("PhantomData marker field compiles and stays zero sized", () => {
        var holder = PhantomHolder<'static, int> {
            marker : std::PhantomData<&'static int> {},
            value : 5
        }
        return holder.value == 5 && sizeof(std::PhantomData<&'static int>) == 0
    })
}
