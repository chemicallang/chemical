using std::string;
using std::string_view;

// ============================================================
// Tests for temporary lifetime extension of .to_view() on
// function return values.
//
// Compiler bug: For chained-temp expressions like
//   consume(func_returning_string().to_view())
// the generated C declares the temp string at function scope but
// places the destructor AFTER `return` (dead code). This means
// the destructor never runs — memory leaks but view stays valid.
//
// For struct-returning functions with custom destructors, the
// destructor also goes after `return` (dead code), so the dtor
// count stays 0 instead of 1. These tests fail until the
// compiler places destructors before `return` for chained temps.
// ============================================================

// ===== Tracked string-like struct =====

struct TempString {
    var data : string
    var dtor_flag : *mut int = null

    func assign(&mut self, s : string_view, flag : *mut int) {
        self.data.append_view(s)
        self.dtor_flag = flag
    }

    func to_view(&self) : string_view {
        return self.data.to_view()
    }

    @delete
    func delete(&mut self) {
        *self.dtor_flag = *self.dtor_flag + 1
    }
}

func make_temp(s : string_view, flag : *mut int) : TempString {
    var t = TempString()
    t.data.append_view(s)
    t.dtor_flag = flag
    return t
}

func consume_view(v : string_view) : bool {
    return v.size() > 0 && v.get(0) == 'H'
}

func consume_view_and_capture(v : string_view) : string {
    var out = string()
    out.append_view(v)
    return out
}

// ===== Named-variable tests (should pass) =====

func make_hello_string() : string {
    var result = string()
    result.append_view(string_view("Hello World! Temporary view test data."))
    return result
}

func make_hello_temp(flag : *mut int) : TempString {
    var t : TempString
    t.data.append_view(string_view("Hello World! TempString view test."))
    t.dtor_flag = flag
    return t
}

func test_temp_view_lifetime() {

    // ----- Named-variable patterns (already work) -----
    test("named TempString variable is destructed once", () => {
        var count = 0
        if(true) {
            var t = make_temp(string_view("Hello"), &mut count)
        }
        return count == 1
    })

    test("named TempString.to_view() consumed then destructed once", () => {
        var count = 0
        if(true) {
            var t = make_temp(string_view("Hello"), &mut count)
            var ok = consume_view(t.to_view())
        }
        return count == 1
    })

    test("named string.to_view() consumed in func arg works", () => {
        var s = make_hello_string()
        return consume_view(s.to_view())
    })

    // ----- Chained-temp patterns (BUG: dtor after return = dead code) -----

    test("temp string.to_view() in func arg — destructor called (currently leaks)", () => {
        var ok = consume_view(make_hello_string().to_view())
        return ok
    })

    test("temp TempString.to_view() in func arg — dtor called once (currently leaks)", () => {
        var count = 0
        if(true) {
            var ok = consume_view(make_temp(string_view("Hello"), &mut count).to_view())
        }
        // BUG: dtor is placed after `return` (dead code), never called
        // count should be 1 but is 0 → test FAILS
        return count == 1
    })

    test("temp TempString.to_view() captured into new string — dtor called once", () => {
        var count = 0
        var captured = string()
        if(true) {
            captured = consume_view_and_capture(make_temp(string_view("Hello World! TempString view test."), &mut count).to_view())
        }
        // BUG: dtor is placed after `return`, never called → count is 0, not 1
        // Also captured should contain the correct content
        return count == 1 && captured.equals_view("Hello World! TempString view test.")
    })

    test("multiple chained temp TempString.to_view() dtors all called", () => {
        var count1 = 0
        var count2 = 0
        if(true) {
            var ok = consume_view(make_temp(string_view("First"), &mut count1).to_view()) &&
                     consume_view(make_temp(string_view("Second"), &mut count2).to_view())
        }
        // BUG: both dtors are dead code → count1 == 0, count2 == 0
        return count1 == 1 && count2 == 1
    })
}
