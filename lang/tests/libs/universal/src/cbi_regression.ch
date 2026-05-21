// Regression tests for universal_cbi parser edge cases
// These test patterns that have historically caused parser failures

// 1. URL query string in JSX expression — `?` in string should not confuse parser
#universal UrlQueryInJsx(props) {
    var url = "/api/items?filter=" + props.filter
    return <a href={url}>Link</a>
}

@test
public func universal_url_query_in_jsx(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UrlQueryInJsx filter="test" /> }
    var js = page.getJs()
    if(js.contains("\\\"/api/items?filter=\\\"") || js.contains("/api/items")) {
        env.success("URL with query string in JSX expression works")
    } else {
        env.error("URL query string handling failed")
        env.info(js.data())
    }
}

// 2. Arrow function with block body in JSX .map()
#universal ArrowBlockBody(props) {
    state items = ["a", "b"]
    return <ul>
        {items.map(item => {
            return <li>{item}</li>
        })}
    </ul>
}

@test
public func universal_arrow_block_body_in_jsx(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ArrowBlockBody /> }
    var js = page.getJs()
    if(js.contains("items.value.map")) {
        env.success("arrow block body in JSX map works")
    } else {
        env.error("arrow block body in JSX map failed")
        env.info(js.data())
    }
}

// 3. Variable name with var_ prefix (should not be parsed as keyword+identifier)
#universal VarPrefixVariable(props) {
    var var_result = props.value * 2
    return <div>{var_result}</div>
}

@test
public func universal_var_prefix_variable(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarPrefixVariable value={5} /> }
    var js = page.getJs()
    if(js.contains("var_result") || js.contains("varResult")) {
        env.success("var_ prefix variable name works")
    } else {
        env.error("var_ prefix variable name failed")
        env.info(js.data())
    }
}

// 4. Mustache-style {{ }} in string literals inside JSX expressions
#universal MustacheInString(props) {
    var tmpl = "{{ name }}"
    return <div>{tmpl}</div>
}

@test
public func universal_mustache_in_string(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MustacheInString /> }
    var js = page.getJs()
    if(js.contains("{{") || js.contains("\\\\{\\\\{")) {
        env.success("mustache {{ }} in string works")
    } else {
        env.error("mustache {{ }} in string failed")
        env.info(js.data())
    }
}

// 5. RegExp from string constructor (avoiding regex literal with {})
#universal RegExpFromString(props) {
    var pattern = new RegExp("{{\\s*name\\s*}}", "g")
    return <div>{pattern + ""}</div>
}

@test
public func universal_regexp_from_string(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RegExpFromString /> }
    var js = page.getJs()
    if(js.contains("RegExp")) {
        env.success("RegExp from string constructor works")
    } else {
        env.error("RegExp from string constructor failed")
        env.info(js.data())
    }
}
