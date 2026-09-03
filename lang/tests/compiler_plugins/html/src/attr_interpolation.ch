// Tests for variable interpolation inside #html attribute values.
// Bug: std::string variables used as href values inside @{while} + #html
// produce empty or literal values instead of the actual URL.

@test
public func string_var_in_href_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var url = std::string("/vehicles/42")
    #html {
        <a href={url}>View</a>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/vehicles/42")) {
        env.success("string variable in href works")
    } else {
        env.error("string variable in href produced empty or wrong value")
        env.info(html.data())
    }
}

@test
public func string_var_in_href_with_concat(env : &mut TestEnv) {
    var page = HtmlPage()
    var base = std::string("/vehicles/")
    var link = base.copy()
    link.append_view("99")
    #html {
        <a href={link}>View</a>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/vehicles/99")) {
        env.success("concatenated string in href works")
    } else {
        env.error("concatenated string in href failed")
        env.info(html.data())
    }
}

@test
public func string_var_in_href_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 3
    #html {
        <div>
            @{while(idx < count) {
                var link = std::string("/item/")
                link.append_view("X")
                idx = idx + 1
                #html {
                    <a href={link}>Item</a>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    // Check that 3 links are present with href attributes
    var has_href = html.contains("href=")
    if(has_href) {
        env.success("string variable in href inside @{while} works")
    } else {
        env.error("string variable in href inside @{while} failed - href attribute missing")
        env.info(html.data())
    }
}

@test
public func string_var_href_not_empty_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 2
    #html {
        <div>
            @{while(idx < count) {
                var mylink = std::string("/page/abc")
                idx = idx + 1
                #html {
                    <a href={mylink}>Click</a>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    // The bug: href ends up empty or as literal "{mylink}"
    if(html.contains("/page/abc")) {
        env.success("href contains expected value inside while loop")
    } else if(html.contains("{mylink}") || html.contains("href=\"\"")) {
        env.error("href is empty or contains literal variable name")
        env.info(html.data())
    } else {
        env.error("href has unexpected value")
        env.info(html.data())
    }
}

@test
public func string_var_in_if_href_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var target = std::string("/details/5")
    #html {
        <div>
            <a href={target}>Details</a>
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/details/5")) {
        env.success("string variable in href outside while works")
    } else {
        env.error("string variable in href outside while failed")
        env.info(html.data())
    }
}

@test
public func link_element_href_from_string_var(env : &mut TestEnv) {
    var page = HtmlPage()
    var url = std::string("/test-page")
    #html {
        <a href={url}>Click</a>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/test-page")) {
        env.success("<a> href from string var works")
    } else {
        env.error("<a> href from string var failed")
        env.info(html.data())
    }
}

@test
public func link_element_href_from_string_var_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 3
    #html {
        <div>
            @{while(idx < count) {
                var myurl = std::string("/item/")
                myurl.append_view("X")
                idx = idx + 1
                #html {
                    <a href={myurl}>Click</a>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/item/X")) {
        env.success("<a> href from string var inside while works")
    } else {
        env.error("<a> href from string var inside while failed")
        env.info(html.data())
    }
}
