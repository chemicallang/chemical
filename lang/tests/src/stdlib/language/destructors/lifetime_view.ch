using std::string;
using std::string_view;

// These tests verify that a string_view obtained from a temporary string
// (via .to_view() on a function return value) remains valid when passed
// to consumer functions. This exercises the compiler's temporary lifetime
// extension — the temporary string must outlive the function call that
// consumes the view.
//
// Currently the compiler fails to compile .to_view() on a temporary string
// inside a lambda with: "value with type 'string' does not satisfy type '&string'"
// Once the compiler is fixed, these tests should compile and pass.

func make_hello_string() : string {
    var result = string()
    result.append_view(string_view("Hello World! Temporary view test data."))
    return result
}

func check_view(v : string_view) : bool {
    return v.size() > 10 && v.get(0) == 'H' && v.get(6) == 'W'
}

func check_view_exact(v : string_view, expected : string_view) : bool {
    return v.equals(expected)
}

func capture_view(v : string_view) : string {
    var out = string()
    out.append_view(v)
    return out
}

func test_temp_view_lifetime() {
    // The pattern func_returning_string().to_view() in a function argument
    // must keep the temporary string alive until the callee finishes reading.
    // If the compiler destroys the temporary early, the string_view dangles
    // causing SIGSEGV or garbage data.

    // Test 1: direct function argument
    test("temporary string_view in direct function arg remains valid", () => {
        return check_view(make_hello_string().to_view())
    })

    // Test 2: captured into another string
    test("temporary string_view captured into new string remains valid", () => {
        var out = capture_view(make_hello_string().to_view())
        return out.equals_view("Hello World! Temporary view test data.")
    })

    // Test 3: exact content match
    test("temporary string_view with exact comparison remains valid", () => {
        return check_view_exact(make_hello_string().to_view(), string_view("Hello World! Temporary view test data."))
    })

    // Test 4: nested transform chain — inner function returns string,
    // .to_view() called on it, passed to outer function
    test("temporary string_view from nested transform chain remains valid", () => {
        var out = capture_view(make_hello_string().to_view())
        return out.equals_view("Hello World! Temporary view test data.")
    })

    // Test 5: multiple temporary views in one expression
    test("multiple temporary string_view in same expression remains valid", () => {
        return check_view(make_hello_string().to_view()) && check_view(make_hello_string().to_view())
    })
}
