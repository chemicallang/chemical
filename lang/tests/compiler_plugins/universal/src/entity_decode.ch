#universal JsxEntityDecode(props) {
    return <button>&amp; &rarr; Test</button>
}

@test
public func universal_entity_decoded_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <JsxEntityDecode/>
    }
    var expected = std::string()
    expected.append_view("function universal_lib_test_JsxEntityDecode(props) { return $_ur.createElement(\"button\", {}, ")
    expected.append('`')
    expected.append_view("& ")
    expected.append_view("\\u{2192}")
    expected.append_view(" Test")
    expected.append('`')
    expected.append_view("); }\nwindow.$__uni_dispatch('universal_lib_test_JsxEntityDecode', document.getElementById('u")
    expected.append_view(page.getComponentId(0))
    expected.append_view("'), {});\n")
    view_equals(env, page.getJs(), expected.to_view())
}

@test
public func universal_entity_preserved_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <JsxEntityDecode/>
    }
    var expected = std::string()
    expected.append_view("<div id=\"u")
    expected.append_view(page.getComponentId(0))
    expected.append_view("\"><button>&amp; &rarr; Test</button></div>")
    string_equals(env, page.toStringHtmlOnly(), expected.to_view())
}

