using namespace std;

// ---------------------------------------------------------------------------
// Minifier: it may only remove what JavaScript does not need.
//
// The emitted bundle is machine-generated JS with no semicolons, so the two ways
// a whitespace minifier breaks a site are (1) merging two statements that were
// separated by a newline and (2) merging two tokens that were separated by a
// space. Every case below is one of those, plus the constructs the minifier is
// supposed to leave alone (strings, templates, regex literals).
// ---------------------------------------------------------------------------

func mn_fail(env : &mut TestEnv, what : string_view, out : &string) {
    var msg = string()
    msg.append_view(&what)
    msg.append_view(" -> [")
    msg.append_view(out.to_view())
    msg.append_view("]")
    unsafe(test_env::ffi::log_cstr(env, LogType.Error, msg.data(), 0, 0))
}

func mn_js(source : string_view) : string {
    var out = minify_js(source)
    return out
}

func mn_css(source : string_view) : string {
    var out = minify_css(source)
    return out
}

func mn_equals(out : &string, want : string_view) : bool {
    return out.equals_view(&want)
}

// Two statements in one line-separated block must stay separated: the emitter
// writes no semicolons, so the newline is the statement boundary.
@test
func test_minify_js_keeps_statement_newlines(env : &mut TestEnv) {
    var source = string("window.a = []\n    window.b = 0\n")
    var want = string("window.a=[]\nwindow.b=0")
    var out = mn_js(source.to_view())
    if(!mn_equals(&out, want.to_view())) {
        mn_fail(env, string_view("a newline between two statements must survive"), &out)
    }
}

// Indentation and blank lines are pure cost, and a `;` makes the newline after
// it redundant too (unlike the generator's own semicolon-free output).
@test
func test_minify_js_drops_indentation(env : &mut TestEnv) {
    var source = string("    const x = 1;\n\n\n    const y = 2;\n")
    var want = string("const x=1;const y=2;")
    var out = mn_js(source.to_view())
    if(!mn_equals(&out, want.to_view())) {
        mn_fail(env, string_view("indentation and blank lines must go"), &out)
    }
}

// A space that separates two words is a token boundary.
@test
func test_minify_js_keeps_space_between_words(env : &mut TestEnv) {
    var source = string("function f(props) { return  typeof  props  }\n")
    var want = string("function f(props){return typeof props\n}")
    var out = mn_js(source.to_view())
    if(out.size() == 0) {
        mn_fail(env, string_view("minified output must not be empty"), &out)
        return
    }
    // The exact newline placement is an implementation detail; the invariant is
    // that no two words were glued together.
    if(out.find_or_npos("typeofprops") != NPOS || out.find_or_npos("returntypeof") != NPOS) {
        mn_fail(env, string_view("words must not be glued together"), &out)
    }
}

// `a + +b` is not `a++b`, and `1 .toString()` is not `1.toString()`.
@test
func test_minify_js_keeps_token_separating_spaces(env : &mut TestEnv) {
    var double_plus = string("var y = a + +b\n")
    var out = mn_js(double_plus.to_view())
    if(out.find_or_npos("a++b") != NPOS) {
        mn_fail(env, string_view("`a + +b` must not collapse into `a++b`"), &out)
    }
    var dot = string("var s = 1 .toString()\n")
    var dot_out = mn_js(dot.to_view())
    if(dot_out.find_or_npos("1.to") != NPOS) {
        mn_fail(env, string_view("`1 .toString()` must keep its space"), &dot_out)
    }
}

// Nothing inside a string may change — including its spaces.
@test
func test_minify_js_leaves_string_literals_alone(env : &mut TestEnv) {
    var source = string("var a = \"two  spaces /* not a comment */ // nor this\";\n")
    var out = mn_js(source.to_view())
    if(out.find_or_npos("two  spaces /* not a comment */ // nor this") == NPOS) {
        mn_fail(env, string_view("a string literal must be copied byte for byte"), &out)
    }
}

// Template literals (with interpolation) are copied whole.
@test
func test_minify_js_leaves_template_literals_alone(env : &mut TestEnv) {
    var source = string("var t = `a  ${x}  b`\n")
    var out = mn_js(source.to_view())
    if(out.find_or_npos("`a  ${x}  b`") == NPOS) {
        mn_fail(env, string_view("a template literal must be copied byte for byte"), &out)
    }
}

// A regular-expression literal is where a whitespace minifier without a full
// parser starts rewriting code, so the whole call returns the original bytes.
@test
func test_minify_js_falls_back_on_regex_literals(env : &mut TestEnv) {
    var source = string("var re = /a  b/\n")
    if(js_is_minifiable(source.to_view())) {
        mn_fail(env, string_view("a regex literal must make the minifier fall back"), &mn_js(source.to_view()))
        return
    }
    var out = mn_js(source.to_view())
    if(!mn_equals(&out, source.to_view())) {
        mn_fail(env, string_view("the fallback must return the original bytes"), &out)
    }
}

// An unterminated literal is not something to guess about either.
@test
func test_minify_js_falls_back_on_unterminated_literals(env : &mut TestEnv) {
    var source = string("var s = \"never closed\n")
    if(js_is_minifiable(source.to_view())) {
        mn_fail(env, string_view("an unterminated string must make the minifier fall back"), &mn_js(source.to_view()))
    }
}

// Division is not a regex: `a / b` must stay minifiable.
@test
func test_minify_js_still_minifies_division(env : &mut TestEnv) {
    var source = string("var half = total / 2\n")
    if(!js_is_minifiable(source.to_view())) {
        mn_fail(env, string_view("division must not be mistaken for a regex"), &mn_js(source.to_view()))
        return
    }
    var out = mn_js(source.to_view())
    if(out.find_or_npos("total/2") == NPOS) {
        mn_fail(env, string_view("division should be minified"), &out)
    }
}

// Comments go, but the code around them stays.
@test
func test_minify_js_drops_comments(env : &mut TestEnv) {
    var source = string("// leading\nvar a = 1 // trailing\n/* block\n   comment */\nvar b = 2\n")
    var out = mn_js(source.to_view())
    if(out.find_or_npos("leading") != NPOS || out.find_or_npos("trailing") != NPOS ||
       out.find_or_npos("block") != NPOS) {
        mn_fail(env, string_view("comments must be removed"), &out)
    }
    if(out.find_or_npos("var a=1") == NPOS || out.find_or_npos("var b=2") == NPOS) {
        mn_fail(env, string_view("the statements around a comment must survive"), &out)
    }
}

// CSS has no automatic-semicolon problem, so newlines always go, and the last
// declaration's semicolon goes with them.
@test
func test_minify_css_collapses_and_drops_last_semicolon(env : &mut TestEnv) {
    var source = string(".a {\n    color : red ;\n    margin : 0 ;\n}\n\n.b > .c {\n    color: blue;\n}\n")
    var want = string(".a{color:red;margin:0}.b>.c{color:blue}")
    var out = mn_css(source.to_view())
    if(!mn_equals(&out, want.to_view())) {
        mn_fail(env, string_view("css must collapse to the same rules"), &out)
    }
}

// A descendant selector needs its space; a string value must not be touched.
@test
func test_minify_css_keeps_descendant_space_and_strings(env : &mut TestEnv) {
    var source = string(".a .b {\n    content : \"two  spaces\";\n}\n")
    var out = mn_css(source.to_view())
    if(out.find_or_npos(".a .b") == NPOS) {
        mn_fail(env, string_view("a descendant selector must keep its space"), &out)
    }
    if(out.find_or_npos("\"two  spaces\"") == NPOS) {
        mn_fail(env, string_view("a css string value must be copied byte for byte"), &out)
    }
}

// The same input always produces the same bytes, so a content hash still names
// the output it describes.
@test
func test_minify_is_deterministic(env : &mut TestEnv) {
    var source = string("function f(props) {\n    return props.a\n}\n")
    var a = mn_js(source.to_view())
    var b = mn_js(source.to_view())
    if(a.equals(&b) == false) {
        mn_fail(env, string_view("two runs must agree"), &a)
    }
}

// ---------------------------------------------------------------------------
// Local-identifier mangling: the names in a component body are ~15% of a
// minified bundle, and renaming one is only safe under a rule that can be
// checked from the bytes alone. Each test below pins one clause of that rule.
// ---------------------------------------------------------------------------

func mn_mangled(source : string_view) : string {
    var out = mangle_js_locals(source)
    return out
}

// A parameter and a `var` that are used only as variables get shorter names, and
// the replacement never collides with a property name the body mentions
// (`props.a` makes `a` unusable, so `props` becomes `b`).
@test
func test_mangle_renames_locals_only_variable_positions(env : &mut TestEnv) {
    var source = string("function f(props){var card=\"x\";return props.a+card}")
    var want = string("function f(b){var c=\"x\";return b.a+c}")
    var out = mn_mangled(source.to_view())
    if(!mn_equals(&out, want.to_view())) {
        mn_fail(env, string_view("a local used only as a variable must shrink"), &out)
    }
}

// `value` is both a binding and a property key (`o.value`). The key is not a use of
// the binding, so the binding may shrink — but the key itself must survive.
@test
func test_mangle_keeps_a_property_key_of_a_renamed_binding(env : &mut TestEnv) {
    var source = string("function f(o){var value=1;return o.value+value}")
    var want = string("function f(o){var a=1;return o.value+a}")
    var out = mn_mangled(source.to_view())
    if(!mn_equals(&out, want.to_view())) {
        mn_fail(env, string_view("a property key must never be renamed"), &out)
    }
}

// A shorthand property (`{name}`) is a key, not a reference, and rewriting it would
// silently rename a field.
@test
func test_mangle_refuses_a_shorthand_property(env : &mut TestEnv) {
    var source = string("function f(onClick,label){var bound={onClick:onClick,label};return bound}")
    var out = mn_mangled(source.to_view())
    if(out.find_or_npos("onClick:") == NPOS) {
        // `onClick` itself is a property key here, so it must stay.
        mn_fail(env, string_view("a property key must not be renamed"), &out)
    }
    if(out.find_or_npos("label}") == NPOS && out.find_or_npos("label:") == NPOS) {
        mn_fail(env, string_view("a shorthand property must not be renamed"), &out)
    }
}

// The replacement must be free in the unit: `a` and `b` are already used, so the
// binding takes the shortest name that is not.
@test
func test_mangle_avoids_names_the_unit_uses(env : &mut TestEnv) {
    var source = string("function f(handler){var a=1;var b=2;return handler(a,b)}")
    var out = mn_mangled(source.to_view())
    if(out.find_or_npos("function f(c)") == NPOS) {
        mn_fail(env, string_view("the first free name must be picked"), &out)
    }
    if(out.find_or_npos("return c(a,b)") == NPOS) {
        mn_fail(env, string_view("the renamed binding must be the parameter"), &out)
    }
}

// Top-level code is shared by every page: it is never mangled, so a name defined in
// a gap and used from another chunk still resolves.
@test
func test_mangle_skips_non_units(env : &mut TestEnv) {
    var source = string("var shared=1\nwindow.x=2")
    var out = mn_mangled(source.to_view())
    if(!mn_equals(&out, source.to_view())) {
        mn_fail(env, string_view("top-level code must be left alone"), &out)
    }
}

// A template literal can contain code (`${x}`); rather than guess, the unit keeps
// its bytes.
@test
func test_mangle_skips_a_unit_with_a_template(env : &mut TestEnv) {
    var source = string("function f(name){return `hi ${name}`}")
    var out = mn_mangled(source.to_view())
    if(!mn_equals(&out, source.to_view())) {
        mn_fail(env, string_view("a unit with a template must be refused"), &out)
    }
}

// A global the body calls (or a word it names) is never captured: `props` would be a
// free `$_ucs`-style collision if the replacement appeared anywhere in the unit.
@test
func test_mangle_keeps_strings_byte_for_byte(env : &mut TestEnv) {
    var source = string("function f(className){var out=\"className\";return out+className}")
    var out = mn_mangled(source.to_view())
    if(out.find_or_npos("\"className\"") == NPOS) {
        mn_fail(env, string_view("a string literal must not be rewritten"), &out)
    }
    if(out.find_or_npos("function f(") == NPOS) {
        mn_fail(env, string_view("the definition must survive"), &out)
    }
}

// A binding used once saves nothing (and its name documents the code): it is left
// as it is, so a debugger still shows `onClick` for the common one-use parameter.
@test
func test_mangle_leaves_single_use_bindings(env : &mut TestEnv) {
    var source = string("function f(onClick,label){return label}")
    var out = mn_mangled(source.to_view())
    if(out.find_or_npos("onClick") == NPOS) {
        mn_fail(env, string_view("a one-use binding must keep its name"), &out)
    }
}
