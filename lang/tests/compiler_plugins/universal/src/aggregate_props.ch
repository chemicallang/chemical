// Aggregate props (§3.15): a `vector<std::string>` / `vector<std::string_view>`
// prop is an aggregate the client can rebuild from the payload. Before this, such
// a prop failed the build with "its type has no JS serialization", so an app could
// render a list server-side but never hand the same list to the hydrated
// component. A literal `options={["a","b"]}` array already worked; this is the
// runtime-vector half of the same contract.

public struct AggregateListProps {
    var items : std::vector<std::string>
}

public struct AggregateViewListProps {
    var items : std::vector<std::string_view>
}

#universal AggregateList(props : AggregateListProps) {
    return <ul data-testid="aggregate-list">list</ul>
}

#universal AggregateViewList(props : AggregateViewListProps) {
    return <ul data-testid="aggregate-view-list">list</ul>
}

@test
public func universal_aggregate_props_reach_the_client_payload(env : &mut TestEnv) {
    var page = HtmlPage()
    var fruits = std::vector<std::string>()
    fruits.push(std::string("apple"))
    fruits.push(std::string("banana"))
    var views = std::vector<std::string_view>()
    views.push(std::string_view("one"))
    views.push(std::string_view("two"))
    #html {
        <AggregateList items={fruits} />
        <AggregateViewList items={views} />
    }
    const html = page.getHtml()
    if(!has_sub(html, std::string_view("data-testid=\"aggregate-list\""))) {
        env.error("aggregate props: the component did not render server-side")
        return
    }
    const js = page.getJs()
    if(!has_sub(js, std::string_view("items\":['apple','banana']"))) {
        env.error("aggregate props: a vector<string> prop is missing from the client payload")
        return
    }
    if(!has_sub(js, std::string_view("items\":['one','two']"))) {
        env.error("aggregate props: a vector<string_view> prop is missing from the client payload")
        return
    }
}

// Escaping is the scalar-string rule: a quote is backslash-escaped and `</` is
// written as `\u003C/`, so a prop value can never close the script tag it is
// embedded in, and the browser decodes the original string back.
@test
public func universal_aggregate_prop_strings_are_escaped(env : &mut TestEnv) {
    var page = HtmlPage()
    var items = std::vector<std::string>()
    items.push(std::string("it's"))
    items.push(std::string("</script>"))
    #html {
        <AggregateList items={items} />
    }
    const js = page.getJs()
    if(!has_sub(js, std::string_view("'it\\'s'"))) {
        env.error("aggregate props: a quote in an element was not escaped")
        return
    }
    if(!has_sub(js, std::string_view("\\u003C/script>"))) {
        env.error("aggregate props: `</script>` in an element was not escaped")
    }
}
