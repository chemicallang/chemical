// Copyright (c) Chemical Language Foundation 2026.

using std::string_view;
using std::string;
using core::ops::Clone;

struct CloneRow {
    var x : int
}

impl Clone for CloneRow {
    func clone(&self) : CloneRow {
        return CloneRow { x : self.x }
    }
}

func <T> duplicate_value(x : T) : T where T: Clone {
    return x.clone()
}

interface Show {
    func show(&self) : int
}

struct ShowRow {
    var x : int
}

impl Show for ShowRow {
    func show(&self) : int {
        return self.x
    }
}

func <T> call_show(x : T) : int where T: Show {
    return x.show()
}

struct ShowWrapper<T> {
    var value : T
}

impl<T> Show for ShowWrapper<T> where T: Show {
    func show(&self) : int {
        return self.value.show()
    }
}

struct CloneWrapper<T> {
    var value : T
}

impl<T> Clone for CloneWrapper<T> where T: Clone {
    func clone(&self) : CloneWrapper<T> {
        return CloneWrapper<T> { value : self.value.clone() }
    }
}

func test_clone_impls() {
    test("int clone", () => {
        var x = 41
        return x.clone() == 41
    })
    test("bool clone", () => {
        var x = true
        return x.clone()
    })
    test("string_view clone", () => {
        var s = string_view("hello")
        var t = s.clone()
        return t.size() == 5 && t.get(0) == 'h'
    })
    test("string deep clone", () => {
        var s = string("hello")
        var t = s.clone()
        return t.size() == 5 && t.get(0) == 'h'
    })
    test("generic clone dispatch for a primitive", () => {
        return duplicate_value(41) == 41
    })
    test("generic clone dispatch for a user type", () => {
        var r = CloneRow { x : 41 }
        var c = duplicate_value(r)
        return c.x == 41
    })
    test("generic interface method dispatch", () => {
        var r = ShowRow { x : 41 }
        return call_show(r) == 41
    })
    test("generic field method dispatch in an impl", () => {
        var w = ShowWrapper<ShowRow> { value : ShowRow { x : 41 } }
        return w.show() == 41
    })
    test("generic field clone in an impl", () => {
        var w = CloneWrapper<CloneRow> { value : CloneRow { x : 41 } }
        var c = w.clone()
        return c.value.x == 41
    })
}
