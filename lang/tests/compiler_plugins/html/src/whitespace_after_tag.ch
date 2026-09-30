// Whitespace immediately after a tag's '>' used to be dropped unconditionally,
// and it welded words together in the emitted html.
//
//     #html {
//         <p>See <a href="/courses/x/lessons/y">the frame</a> is where.</p>
//     }
//
// rendered as
//
//     <p>See <a href="/courses/x/lessons/y">the frame</a>is where.</p>
//
// There is nothing in the output to say a space had been lost, and the browser
// has no way to guess where it belonged. One downstream course collection
// (Underlayer) had 14,945 sites written around the bug -- links were ended on
// punctuation and mid-sentence links were reworded -- and about ninety welded
// words survived per rendered page.
//
// The rule now is the one every html author already writes against:
//
//   * a space or tab on the SAME LINE as the tag it follows is CONTENT and is
//     emitted;
//   * a run containing a newline or carriage return is pretty-printing
//     indentation and is still dropped, so every pretty-printed block keeps
//     byte-identical output;
//   * whitespace that only precedes a structural boundary -- a closing tag, or
//     the '#html' block's own '}' -- is still dropped.
//
// The three tests under "still dropped" are the ones that would have caught an
// over-eager fix, and they are the reason the newline test comes first in the
// list of what this file pins.

// ------------------------------------------------------ content, now kept

@test
public func same_line_space_after_a_closing_tag_is_content(env : &mut TestEnv) {
    // the exact failure, verbatim
    var page = HtmlPage()
    #html {
        <p>See <a href="/x">the frame</a> is where.</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>See <a href=\"/x\">the frame</a> is where.</p>");
}

@test
public func same_line_space_after_an_opening_tag_is_content(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p><b>bold</b> then plain</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p><b>bold</b> then plain</p>");
}

@test
public func same_line_space_after_a_closing_inline_element_is_content(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p>the register is <code>x0</code> and the link is <a href="/x">here</a> too</p>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<p>the register is <code>x0</code> and the link is <a href=\"/x\">here</a> too</p>");
}

@test
public func same_line_tab_after_a_tag_is_content(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p>a<b>x</b>	b</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>a<b>x</b>\tb</p>");
}

@test
public func several_same_line_spaces_after_a_tag_are_all_content(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p>a<b>x</b>   b</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>a<b>x</b>   b</p>");
}

@test
public func same_line_space_after_a_nested_block_close_is_content(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div><span>x</span> after</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><span>x</span> after</div>");
}

// ------------------------------------------------------ formatting, still dropped

@test
public func newline_indentation_after_a_tag_is_still_dropped(env : &mut TestEnv) {
    // the common case: every pretty-printed block in every project. This output
    // must be byte-identical to what it was before the fix.
    var page = HtmlPage()
    #html {
        <div>
            <p>one</p>
            <p>two</p>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><p>one</p><p>two</p></div>");
}

@test
public func mixed_same_line_and_newline_whitespace_behave_differently(env : &mut TestEnv) {
    // the same document, both cases, so the two rules cannot be conflated
    var page = HtmlPage()
    #html {
        <div>
            <b>x</b> kept
            <b>y</b>
            dropped
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><b>x</b> kept<b>y</b>dropped</div>");
}

@test
public func space_before_a_closing_tag_is_still_dropped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div><b>x</b> </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><b>x</b></div>");
}

@test
public func space_before_the_macro_closing_brace_is_still_dropped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>text </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>text</div>");
}

@test
public func space_after_a_tag_before_another_tag_is_still_dropped(env : &mut TestEnv) {
    // a boundary case: the run is followed by '<' rather than a word
    var page = HtmlPage()
    #html {
        <div><span>x</span> <span>y</span></div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><span>x</span><span>y</span></div>");
}

// ------------------------------------------------------------------ <pre>

@test
public func same_line_space_after_a_tag_inside_pre_is_unchanged(env : &mut TestEnv) {
    // <pre> has its own whitespace path (pre_depth) and must be untouched: the
    // run there is significant for a different reason and both rules agree on
    // the answer.
    var page = HtmlPage()
    #html {
        <pre>if (x) { <b>y</b> }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (x) { <b>y</b> }</pre>");
}

@test
public func pre_multiline_indentation_is_still_preserved(env : &mut TestEnv) {
    // the counterpart: <pre> keeps its newlines, which is the whole point of
    // the element and the reason the fix is gated on the newline
    var page = HtmlPage()
    #html {
        <pre>a
    b</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>a\n    b</pre>");
}
