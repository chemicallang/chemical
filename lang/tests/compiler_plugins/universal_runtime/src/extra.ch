@test
public func test_universal_multiple_statements(env : &mut TestEnv) {
    const view = std::string_view("var a = 1; var b = 2; return a + b;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("var a = 1;var b = 2;return a + b;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_arrow_block_body(env : &mut TestEnv) {
    const view = std::string_view("const f = (x) => { return x * 2; };")
    var out = universal::parse_universal(view)
    var expected = std::string_view("const f = (x) => {return x * 2;};")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_conditional_expression(env : &mut TestEnv) {
    const view = std::string_view("return <p>{cond ? \"yes\" : \"no\"}</p>;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("return <p>{(cond ? \"yes\" : \"no\")}</p>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_single_quote_attribute(env : &mut TestEnv) {
    const view = std::string_view("<div class='single'>x</div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div class='single'>x</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_nested_components(env : &mut TestEnv) {
    const view = std::string_view("<Outer><Inner>text</Inner></Outer>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<Outer><Inner>text</Inner></Outer>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_map_jsx(env : &mut TestEnv) {
    const view = std::string_view("return <div>{items.map(x => <li>{x}</li>)}</div>;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("return <div>{items.map((x) => <li>{x}</li>)}</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_event_handler(env : &mut TestEnv) {
    const view = std::string_view("<button onClick={handle}>Click</button>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<button onClick={handle}>Click</button>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_conditional_and_jsx(env : &mut TestEnv) {
    const view = std::string_view("<div>{cond && <span>shown</span>}</div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div>{cond && <span>shown</span>}</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_map_with_key(env : &mut TestEnv) {
    const view = std::string_view("return <ul>{items.map(x => <li key={x.id}>{x.name}</li>)}</ul>;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("return <ul>{items.map((x) => <li key={x.id}>{x.name}</li>)}</ul>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_arithmetic(env : &mut TestEnv) {
    const view = std::string_view("var total = a + b * c;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("var total = a + b * c;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_two_attributes(env : &mut TestEnv) {
    const view = std::string_view("<div className=\"a\" id=\"b\">x</div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div className=\"a\" id=\"b\">x</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_expression_text(env : &mut TestEnv) {
    const view = std::string_view("return <div>{count}</div>;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("return <div>{count}</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_array_map(env : &mut TestEnv) {
    const view = std::string_view("const arr = [1, 2]; return arr.map(x => x * 2);")
    var out = universal::parse_universal(view)
    var expected = std::string_view("const arr = [1, 2];return arr.map((x) => x * 2);")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_modulo(env : &mut TestEnv) {
    const view = std::string_view("var rem = n % 3; return rem;")
    var out = universal::parse_universal(view)
    var expected = std::string_view("var rem = n % 3;return rem;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_switch_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("switch(kind) { case \"a\": label = \"A\"; break; default: label = \"D\"; }")
    var out = universal::parse_universal(view)
    var expected = std::string_view("switch(kind) {case \"a\":label = \"A\";break;default:label = \"D\";}")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_while_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("while(i < n) { i++; }")
    var out = universal::parse_universal(view)
    var expected = std::string_view("while(i < n){i++;}")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_throw_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("if(bad) throw \"boom\";")
    var out = universal::parse_universal(view)
    var expected = std::string_view("if(bad)throw \"boom\";")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_do_while_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("do { i++; } while(i < n);")
    var out = universal::parse_universal(view)
    var expected = std::string_view("do {i++;} while(i < n);")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_for_of_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("for(var item of items) { out += item; }")
    var out = universal::parse_universal(view)
    var expected = std::string_view("for(var item of items){out += item;}")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_class_roundtrip(env : &mut TestEnv) {
    const view = std::string_view("class Point { constructor(x, y) { this.x = x; } }")
    var out = universal::parse_universal(view)
    var expected = std::string_view("class Point {constructor(x, y) {this.x = x;}}")
    univ_view_equals(env, out.to_view(), &expected)
}
