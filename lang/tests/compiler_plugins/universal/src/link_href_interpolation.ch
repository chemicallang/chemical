// Tests for Link component href interpolation.
// Local Link matches the real implementation from lang/libs/components/src/Typography.ch:
//   public #universal Link(props) {
//       return <a href={props.href}>{props.children}</a>
//   }
//
// BUG: When a std::string variable is passed as href to a #universal component,
// the href attribute ends up empty in the rendered HTML. Literal strings work.
// The root cause is in the universal CBI prop value handling — variable props
// are not properly passed through to the inner <a> tag's href attribute.

#universal Link(props) {
    return <a href={props.href}>{props.children}</a>
}

@test
public func link_href_from_string_var(env : &mut TestEnv) {
    var page = HtmlPage()
    var url = std::string("/test-page")
    #html {
        <Link href={url}>Click</Link>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/test-page")) {
        env.success("Link href from string var works")
    } else {
        env.error("Link href from string var failed - href is empty or missing")
        env.info(html.data())
    }
}

@test
public func link_href_from_string_var_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 3
    #html {
        <div>
            @{while(idx < count) {
                var myurl = std::string("/item/X")
                idx = idx + 1
                #html {
                    <Link href={myurl}>Click</Link>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/item/X")) {
        env.success("Link href from string var inside while works")
    } else {
        env.error("Link href from string var inside while failed - href is empty")
        env.info(html.data())
    }
}

@test
public func link_href_from_concatenated_string(env : &mut TestEnv) {
    var page = HtmlPage()
    var base = std::string("/vehicles/")
    var link = base.copy()
    link.append_view("42")
    #html {
        <Link href={link}>View</Link>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("/vehicles/42")) {
        env.success("Link href from concatenated string works")
    } else {
        env.error("Link href from concatenated string failed")
        env.info(html.data())
    }
}

@test
public func link_href_not_empty_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 2
    #html {
        <div>
            @{while(idx < count) {
                var link = std::string("/page/abc")
                idx = idx + 1
                #html {
                    <Link href={link}>Click</Link>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    // The bug: href ends up as href="" when a variable is passed
    if(html.contains("/page/abc")) {
        env.success("Link href contains expected value inside while loop")
    } else if(html.contains("href=\"\"")) {
        env.error("BUG CONFIRMED: Link href is empty when variable is passed as prop")
        env.info(html.data())
    } else {
        env.error("Link href has unexpected value")
        env.info(html.data())
    }
}

@test
public func link_href_literal_string_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 3
    #html {
        <div>
            @{while(idx < count) {
                idx = idx + 1
                #html {
                    <Link href="/item/static">Link</Link>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    // Literal strings work fine — only variable props are broken
    if(html.contains("/item/static")) {
        env.success("Link with literal href inside while works")
    } else {
        env.error("Link with literal href inside while failed")
        env.info(html.data())
    }
}

@test
public func raw_a_href_from_string_var_inside_while(env : &mut TestEnv) {
    var page = HtmlPage()
    var idx : size_t = 0
    var count : size_t = 3
    #html {
        <div>
            @{while(idx < count) {
                var myurl = std::string("/item/raw")
                idx = idx + 1
                #html {
                    <a href={myurl}>Click</a>
                }
            }}
        </div>
    }
    var html = std::string()
    html.append_view(page.getHtml())
    // Raw <a> tags work — only #universal component prop passing is broken
    if(html.contains("/item/raw")) {
        env.success("Raw <a> href from string var inside while works")
    } else {
        env.error("Raw <a> href from string var inside while failed")
        env.info(html.data())
    }
}
