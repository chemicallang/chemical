// dialect_regressions.ch — valid JavaScript the `#js` emitter gets wrong.
//
// WHY THIS FILE EXISTS
// --------------------
// Every case here is VALID JavaScript that a browser runs correctly, and every
// one of them was found the expensive way: by writing it inside a product's
// `runtime/*.ch`, getting a green build, and shipping something that did not
// work. Two of them are the worst kind of compiler bug there is — the build
// SUCCEEDS and the emitted bundle is valid JavaScript that means something else,
// or is missing a function entirely.
//
// These are regressions of bugs that were found the expensive way and have since
// been fixed (the parser now preserves explicit grouping parentheses in every
// mode). They are written from the expectation, not from the current output: a
// test that agrees with a bug protects nothing.
//
// The parse-time failures — valid JS the parser REFUSES outright — are not here,
// because one rejected statement fails the whole module and takes the working
// cases with it.
//
// The idiom is `page.toStringJsOnly()`, as in to_string.ch: the emitted text is
// the thing under test, because the bug is in what comes out.

using std::string;
using std::string_view;

// ── 1. An object literal inside a function body is dropped ENTIRELY ────────
//
// `to_string.ch::object_literal_works` already covers an object literal at the
// top level of a `#js` block, and it passes. The gap is one scope deeper, and it
// is the worst failure mode in this file: the build is green, `page.toStringJsOnly()`
// comes back valid, and the FUNCTION IS SIMPLY NOT THERE.
//
// Found in Web/timeline: a `#globaljs` runtime whose
// `$timeline_resolve_row` began `var d = { action: "TakeServer", conflict: false,
// push: false }`. The whole function vanished from `/app.js`. Every test in the
// product's suite still passed, because the product's tests exercise behaviour
// through a page and a missing function only shows up as `undefined` at runtime.
@test
public func object_literal_inside_a_function_body_is_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        function decide(local) {
            var decision = { action: "TakeServer", conflict: false, push: false };
            return decision;
        }
    }
    string_equals(
        env,
        page.toStringJsOnly(),
        "function decide(local){var decision = { action: \"TakeServer\", conflict: false, push: false };return decision;}"
    );
}

// ── 2. The same thing via `return` ─────────────────────────────────────────
//
// Reported separately because the fix for (1) could plausibly be "parse the
// object literal only in a `var` initialiser", which would leave this one broken.
// `return { a: 1 }` is the single most common way an object literal appears in
// real code.
@test
public func object_literal_as_a_return_value_is_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        function make() {
            return { id: "b1", type: "list" };
        }
    }
    string_equals(
        env,
        page.toStringJsOnly(),
        "function make(){return { id: \"b1\", type: \"list\" };}"
    );
}

// ── 3. Grouping parentheses are lost in OPERAND position ───────────────────
//
// The single most expensive bug found in this codebase, and the reason this file
// exists. `!(id in local)` becomes `!id in local`, because `!` binds tighter
// than `in`, so that is `((!id) in local)` — always false.
//
// The consequence was that Web/timeline's `$timeline_sync_changed_ids` returned
// `[]` for every input, so the browser NEVER ONCE downloaded a note from the
// server. Nothing errored. The app looked completely healthy and held nothing.
//
// Note what survives and what does not, because the fix is not "parenthesise
// more": ternaries and call arguments ARE re-parenthesised correctly. What is
// lost is a parenthesised group used as an operand of a unary or arithmetic
// operator. Case 4 is the control that proves the distinction is real.
@test
public func a_group_in_unary_operand_position_keeps_its_parentheses(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var wanted = [];
        var id = "a1";
        var local = { a1: true };
        if(!(id in local)) { wanted.push(id); }
    }
    string_equals(
        env,
        page.toStringJsOnly(),
        "var wanted = [];var id = \"a1\";var local = { a1: true };if(!(id in local)){wanted.push(id);}"
    );
}

// ── 4. The control: a parenthesised condition keeps its group ──────────────
//
// If this one were broken too, case 3's expectation would be wrong rather than
// the emitter being wrong. A regression test for a compiler bug needs its own
// control.
//
// The emitted text keeps the explicit `(1 > 0)` group AND the emitter's
// established ternary parenthesisation (`Ternary` always emits its own
// parens — see `ternary_operator_work`), so the result is `((1 > 0) ? 4 : 9)`:
// semantically exact, just conservatively parenthesised.
@test
public func a_parenthesised_ternary_condition_keeps_its_group(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var at = (1 > 0) ? 4 : 9;
    }
    string_equals(env, page.toStringJsOnly(), "var at = ((1 > 0) ? 4 : 9);");
}

// ── 5. Arithmetic: a parenthesised group as an operand ──────────────────────
//
// The other half of case 3's damage. `a + (x || "d") + b` becomes
// `a + x || "d" + b`, which is ONE short-circuiting `||` — so the function
// returned only its first term and the rest of the expression vanished without
// a word.
//
// `a & (x + b)` is the shape that bit: `&` is lower-precedence than `+`, so
// dropping the group silently changes what the expression means.
//
// With the group preserved the emitter produces `a & (x || d) + b`, which
// JavaScript parses as `a & ((x || d) + b)` because `+` binds tighter than `&`
// — exactly the source's meaning. No extra outer parens are needed.
@test
public func a_group_in_arithmetic_operand_position_keeps_its_parentheses(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var flags = 0;
        var a = 1;
        var x = 0;
        var d = 2;
        var b = 4;
        var sum = a & (x || d) + b;
    }
    string_equals(
        env,
        page.toStringJsOnly(),
        "var flags = 0;var a = 1;var x = 0;var d = 2;var b = 4;var sum = a & (x || d) + b;"
    );
}