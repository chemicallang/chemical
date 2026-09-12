// =============================================================================
// Feature Regression Tests: Component Lifecycle & Universal Features
//
// Pins component boundary behavior, dedup, state, hooks, and edge cases.
// These are @test functions that use #universal components and #html blocks.
// =============================================================================

// =============================================================================
// Component Boundaries & SSR
// =============================================================================

@test
public func feature_boundary_has_data_chx_i_attribute(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Greeting /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-chx-i")) {
        env.success("component SSR boundary has data-chx-i attribute")
    } else {
        env.error("missing data-chx-i on boundary")
        env.info(html.data())
    }
}

@test
public func feature_boundary_id_is_unique_per_instance(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
        <Greeting />
    }
    var id0 = page.getComponentId(0)
    var id1 = page.getComponentId(1)
    if(!id0.equals(&id1)) {
        env.success("two component instances get unique boundary IDs")
    } else {
        env.error("duplicate boundary IDs")
    }
}

@test
public func feature_boundary_span_wraps_component_html(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Greeting /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // The boundary should be: <span id="uN" data-chx-i>...component HTML...</span>
    if(html.contains("data-chx-i") && html.contains("Hello")) {
        env.success("boundary span wraps the component's rendered HTML")
    } else {
        env.error("boundary span structure incorrect")
        env.info(html.data())
    }
}

@test
public func feature_component_dedup_function_emitted_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
        <Greeting />
    }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("function universal_lib_test_Greeting(")) {
        env.success("component function is present in JS output")
    } else {
        env.error("component function missing from JS output")
        env.info(js.data())
    }
}

@test
public func feature_multiple_component_dispatches_present(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
        <Greeting />
    }
    var js = std::string()
    js.append_view(page.getJs())
    // Two dispatch calls should exist for two component instances
    if(js.contains("__uni_dispatch") && js.contains("Greeting")) {
        env.success("dispatch calls present for multiple component instances")
    } else {
        env.error("dispatch calls missing")
        env.info(js.data())
    }
}

// =============================================================================
// State & Reactivity
// =============================================================================

@test
public func feature_state_renders_initial_value_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStateTernary /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // SsrStateTernary has state open = true, renders "YES"
    if(html.contains("YES")) {
        env.success("state initial value used in SSR ternary")
    } else {
        env.error("state initial value not reflected in SSR")
        env.info(html.data())
    }
}

@test
public func feature_state_map_renders_all_items(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapState /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<li>a</li>") && html.contains("<li>b</li>") && html.contains("<li>c</li>")) {
        env.success("state array .map() renders all items in SSR")
    } else {
        env.error("state array .map() did not render all items")
        env.info(html.data())
    }
}

@test
public func feature_state_map_index_renders_correctly(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapIndexState /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("0:a") && html.contains("1:b") && html.contains("2:c")) {
        env.success("state .map() with index renders index:value pairs")
    } else {
        env.error("state .map() with index incorrect")
        env.info(html.data())
    }
}

@test
public func feature_state_length_renders_count(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrLenState /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("2 items")) {
        env.success("state array .length renders correct count")
    } else {
        env.error("state array .length incorrect")
        env.info(html.data())
    }
}

@test
public func feature_state_concat_renders_all_with_suffix(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapConcat /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("a!") && html.contains("b!") && html.contains("c!")) {
        env.success("state .map() with string concatenation renders correctly")
    } else {
        env.error("state .map() concatenation incorrect")
        env.info(html.data())
    }
}

@test
public func feature_state_numeric_expression_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapNumbers /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<li>2</li>") && html.contains("<li>3</li>") && html.contains("<li>4</li>")) {
        env.success("state .map() with numeric expression renders computed values")
    } else {
        env.error("state .map() numeric expression incorrect")
        env.info(html.data())
    }
}

@test
public func feature_empty_state_array_renders_no_items(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyStateArrayComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("<li>")) {
        env.success("empty state array produces no <li> elements")
    } else {
        env.error("empty state array produced unexpected items")
        env.info(html.data())
    }
}

// =============================================================================
// Props
// =============================================================================

@test
public func feature_props_passed_through_to_component(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DeepPropPassing title="Hover Me" label="Text" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("title=\"Hover Me\"") && html.contains("Text")) {
        env.success("props passed through to nested component")
    } else {
        env.error("props not forwarded correctly")
        env.info(html.data())
    }
}

@test
public func feature_props_array_map_renders_all(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapProps items={["One", "Two"]} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<li>One</li>") && html.contains("<li>Two</li>")) {
        env.success("props array .map() renders all items")
    } else {
        env.error("props array .map() did not render all items")
        env.info(html.data())
    }
}

@test
public func feature_props_length_renders_count(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrLenProps items={["a", "b", "c", "d"]} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("4 items")) {
        env.success("props array .length renders correct count")
    } else {
        env.error("props array .length incorrect")
        env.info(html.data())
    }
}

@test
public func feature_null_prop_skipped_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NullPropComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("null")) {
        env.success("null/undefined props produce no literal 'null' in SSR")
    } else {
        env.error("null prop leaked into SSR output")
        env.info(html.data())
    }
}

@test
public func feature_numeric_props_render_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <AttrChemValue val={42} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-val=\"42\"")) {
        env.success("numeric props render as string in SSR attributes")
    } else {
        env.error("numeric props did not render correctly")
        env.info(html.data())
    }
}

// =============================================================================
// Nested Components
// =============================================================================

@test
public func feature_nested_component_renders_inner_html(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedUniversal /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // NestedUniversal renders <Greeting /> which renders <span>Hello</span>
    if(html.contains("Hello")) {
        env.success("nested component renders inner component's HTML")
    } else {
        env.error("nested component did not render inner HTML")
        env.info(html.data())
    }
}

@test
public func feature_nested_component_with_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedUniversalProps title="T" label="L" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("title=\"T\"") && html.contains("L")) {
        env.success("nested component receives and renders props")
    } else {
        env.error("nested component props not forwarded")
        env.info(html.data())
    }
}

// =============================================================================
// Fragments
// =============================================================================

@test
public func feature_fragment_renders_multiple_children(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragParent /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("Hello") && html.contains("Middle")) {
        env.success("fragment renders all sibling children")
    } else {
        env.error("fragment did not render all children")
        env.info(html.data())
    }
}

// =============================================================================
// Spread Props
// =============================================================================

@test
public func feature_spread_props_forward_all(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadAndStaticAttr title="Hello" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("title=\"Hello\"") && html.contains("id=\"static\"") && html.contains("data-val=\"123\"")) {
        env.success("spread props forwarded alongside static attrs")
    } else {
        env.error("spread props not forwarded correctly")
        env.info(html.data())
    }
}

@test
public func feature_spread_static_overrides_dynamic(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadWithDuplicateAttr title="dynamic" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // SpreadWithDuplicateAttr: <div {...props} title="static">
    // Static title="static" wins over spread title="dynamic"
    if(html.contains("title=\"static\"") && !html.contains("title=\"dynamic\"")) {
        env.success("static attribute overrides spread attribute (last-wins)")
    } else {
        env.error("static did not override spread")
        env.info(html.data())
    }
}

// =============================================================================
// Conditional Rendering (SSR)
// =============================================================================

@test
public func feature_conditional_true_branch_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedTernaryTrueComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("Open")) {
        env.success("state=true renders true branch in SSR")
    } else {
        env.error("state=true did not render true branch")
        env.info(html.data())
    }
}

@test
public func feature_conditional_false_branch_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedTernaryFalseComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("Closed")) {
        env.success("state=false renders false branch in SSR")
    } else {
        env.error("state=false did not render false branch")
        env.info(html.data())
    }
}

// =============================================================================
// Inline Style Objects
// =============================================================================

@test
public func feature_inline_style_object_renders_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStateAttrTernary /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // SsrStateAttrTernary: state open = false, renders style="color:blue;"
    if(html.contains("color:blue")) {
        env.success("inline style object renders correct value in SSR")
    } else {
        env.error("inline style object not rendered in SSR")
        env.info(html.data())
    }
}

// =============================================================================
// JS Output Structure
// =============================================================================

@test
public func feature_js_output_has_function_and_dispatch(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PosTrackingComp x={42} /> }
    var js = std::string()
    js.append_view(page.getJs())
    var has_func = js.contains("function ")
    var has_dispatch = js.contains("window.$__uni_dispatch(")
    if(has_func && has_dispatch) {
        env.success("JS output contains both function definition and dispatch")
    } else {
        env.error("JS output missing function or dispatch")
        env.info(js.data())
    }
}

@test
public func feature_js_state_creates_signal(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StateTest /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_us(")) {
        env.success("state declaration generates $_us() call in JS")
    } else {
        env.error("state declaration missing $_us() in JS output")
        env.info(js.data())
    }
}

@test
public func feature_js_use_effect_registers(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UseEffectDepsTest /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_r.useEffect(")) {
        env.success("useEffect generates $_r.useEffect() call in JS")
    } else {
        env.error("useEffect missing from JS output")
        env.info(js.data())
    }
}

@test
public func feature_js_event_handler_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StateTest /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("onClick")) {
        env.success("event handler emitted in JS output")
    } else {
        env.error("event handler missing from JS output")
        env.info(js.data())
    }
}

// =============================================================================
// Multiple Components on One Page
// =============================================================================

@test
public func feature_multiple_component_types_on_page(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <MultiCompA x="1" />
        <MultiCompB y="2" />
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("A-1") && html.contains("B-2")) {
        env.success("multiple component types render correctly on same page")
    } else {
        env.error("multiple component types did not render correctly")
        env.info(html.data())
    }
}

@test
public func feature_same_component_multiple_times(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <DedupComp label="first" />
        <DedupComp label="second" />
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("first") && html.contains("second")) {
        env.success("same component rendered multiple times with different props")
    } else {
        env.error("multiple instances of same component failed")
        env.info(html.data())
    }
}

// =============================================================================
// Block Body (.map with return)
// =============================================================================

@test
public func feature_map_block_body_renders_all(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapBlockBody /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<li>x</li>") && html.contains("<li>y</li>")) {
        env.success(".map() with block body renders all items")
    } else {
        env.error(".map() block body did not render correctly")
        env.info(html.data())
    }
}

// =============================================================================
// Inline Literal Arrays
// =============================================================================

@test
public func feature_inline_literal_array_map(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrMapInlineLiteral /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<li>10</li>") && html.contains("<li>20</li>")) {
        env.success("inline literal array .map() renders correctly")
    } else {
        env.error("inline literal array .map() did not render correctly")
        env.info(html.data())
    }
}

// =============================================================================
// State Props Init
// =============================================================================

@test
public func feature_state_initialized_from_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrStatePropsInit defaultIndex={2} items={["x", "y", "z"]} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // state active = props.defaultIndex (2), so z should have "on" and x,y should have "off"
    if(html.contains("style=\"on\"") && html.contains("z")) {
        env.success("state initialized from props renders correctly")
    } else {
        env.error("state props init incorrect")
        env.info(html.data())
    }
}

// =============================================================================
// CSS Helpers with Components
// =============================================================================

@test
public func feature_component_with_class_merge(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ClassMerge class="extra" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("base-class") && html.contains("extra")) {
        env.success("component class merge produces both base and extra classes")
    } else {
        env.error("component class merge incorrect")
        env.info(html.data())
    }
}

// =============================================================================
// Nested Component with Component Child
// =============================================================================

@test
public func feature_nested_universal_with_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // ComponentChild renders <div><Greeting /></div> = <div><span>Hello</span></div>
    if(html.contains("Hello")) {
        env.success("nested universal component renders child component's HTML")
    } else {
        env.error("nested universal component child not rendered")
        env.info(html.data())
    }
}

// =============================================================================
// Backtick in Text Content
// =============================================================================

@test
public func feature_backtick_in_text_content_not_interpolated(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BacktickText /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("sync.status") || html.contains("`")) {
        env.success("backtick in text content is preserved literally")
    } else {
        env.error("backtick text content not rendered")
        env.info(html.data())
    }
}

// =============================================================================
// Dollar Identifier in JSX
// =============================================================================

@test
public func feature_dollar_identifier_not_treated_as_interpolation(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DollarIdentifierTest /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("window.$flag") || js.contains("$flag")) {
        env.success("dollar identifier in JSX not treated as interpolation")
    } else {
        env.error("dollar identifier handling incorrect")
        env.info(js.data())
    }
}

// =============================================================================
// typeof Check
// =============================================================================

@test
public func feature_typeof_check_emitted_correctly(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StrictTypeofCheck /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("typeof")) {
        env.success("typeof check emitted in JS output")
    } else {
        env.error("typeof check missing from JS output")
        env.info(js.data())
    }
}

// =============================================================================
// Parenthesized Ternary with Member Access
// =============================================================================

@test
public func feature_parenthesized_ternary_member_access(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ParenthesizedTernaryLabel /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("?") && js.contains(":")) {
        env.success("parenthesized ternary with member access emitted")
    } else {
        env.error("parenthesized ternary not emitted correctly")
        env.info(js.data())
    }
}

// =============================================================================
// Event Handler Nested Through Component
// =============================================================================

@test
public func feature_event_handler_nested_through_component(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventParent /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("onClick")) {
        env.success("event handler passed through nested component")
    } else {
        env.error("event handler not present in JS output")
        env.info(js.data())
    }
}

// =============================================================================
// Props Spread with Event Handler
// =============================================================================

@test
public func feature_spread_with_event_handler(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventParent /> }
    var js = std::string()
    js.append_view(page.getJs())
    // EventParent passes onClick to EventNested
    if(js.contains("onClick") && js.contains("EventNested")) {
        env.success("event handler forwarded through spread")
    } else {
        env.error("event handler forwarding through spread failed")
        env.info(js.data())
    }
}

// =============================================================================
// Multiple State Variables
// =============================================================================

@test
public func feature_multiple_state_variables(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MultiState /> }
    var js = std::string()
    js.append_view(page.getJs())
    // MultiState has state a = 1, state b = 2
    if(js.contains("$_us(")) {
        env.success("multiple state variables generate multiple $_us() calls")
    } else {
        env.error("multiple state variables incorrect")
        env.info(js.data())
    }
}

// =============================================================================
// Empty Component (no children, no content)
// =============================================================================

@test
public func feature_empty_component_renders_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyBlock /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-chx-i") && html.contains("<div></div>")) {
        env.success("empty component renders empty div inside boundary")
    } else {
        env.error("empty component rendering incorrect")
        env.info(html.data())
    }
}

// =============================================================================
// deeplyNested prop access (prop.prop.prop)
// =============================================================================

@test
public func feature_deeply_nested_prop_access(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropValueSugar /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("user.user.name") || js.contains("props.user")) {
        env.success("deeply nested prop access emitted in JS")
    } else {
        env.error("deeply nested prop access missing from JS")
        env.info(js.data())
    }
}

// =============================================================================
// Probed: defaultUniversalSetup content
// =============================================================================

@test
public func feature_css_display_contents_rule(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    var css = std::string()
    css.append_view(page.getCss())
    if(css.contains("data-chx-i") && css.contains("display:contents")) {
        env.success("defaultUniversalSetup adds display:contents for hydration boundaries")
    } else {
        env.error("display:contents CSS rule missing")
        env.info(css.data())
    }
}

@test
public func feature_head_js_has_dispatch_runtime(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    var headJs = std::string()
    headJs.append_view(page.getHeadJs())
    if(headJs.contains("$__uni_hydration_queue") && headJs.contains("$__uni_dispatch")) {
        env.success("headJs contains hydration queue and dispatch runtime")
    } else {
        env.error("headJs missing runtime functions")
        env.info(headJs.data())
    }
}
