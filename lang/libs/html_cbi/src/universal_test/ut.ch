// ---------------------------------------------------------------------------
// #universal_test
//
// `#universal_test("name" [, isolate] [, group = "g"]) {
//      <fixture JSX/HTML, rendered with the production SSR pipeline/>
//      <script> ...raw JS test steps... </script>
//  }`
//
// The fixture is rendered by the exact `#html` converter (ASTConverter), so its
// SSR markup, hydration boundary, client component functions and dispatches are
// identical to a production page. The `<script>` content is captured verbatim
// and emitted into the page's JS bundle as a registered test function.
//
// A top-level test creates `ut_render_<name>(page : &mut HtmlPage)`, which the
// `universal_test` library enumerates via the `universal_test` collector
// annotation + `intrinsics::get_universal_tests<UTFunction>()`, then drives one
// WebView. A statement test renders into the enclosing function's `page`.
// ---------------------------------------------------------------------------

public struct UniversalTestDecl {
    var name : std::string_view
    var group : std::string_view
    var isolate : bool
    var statement : bool
    var params_ready : bool
    var fixture : *mut HtmlRoot
    var steps : std::string_view
    // the generated `ut_render_<name>(page : &mut HtmlPage)` function (top level)
    var fixtureFn : *mut FunctionDeclaration
}

public func ut_known_type(value : *EmbeddedNode) : *BaseType {
    return null
}

public func ut_child_res(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null
}

public func ut_cross_mod(obj : *mut void, node : *mut EmbeddedNode, fn : CrossModuleSymbolDeclarerFn, at_least_spec : AccessSpecifier) {
    // tests are not cross-module symbols
}

// ut_render_<sanitized name>
func ut_sanitize_name(name : std::string_view, builder : *mut ASTBuilder) : std::string_view {
    var s = std::string("ut_render_")
    for(var i : size_t = 0; i < name.size(); i++) {
        const c = name.get(i)
        if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            s.append(c)
        } else {
            s.append('_')
        }
    }
    const v = s.view()
    return builder.allocate_view(&v)
}

func ut_collect_script_text(el : *mut HtmlElement, builder : *mut ASTBuilder) : std::string_view {
    var s = std::string()
    for(var i : size_t = 0; i < el.children.size(); i++) {
        const ch = el.children.get(i)
        if(ch.kind == HtmlChildKind.Text) {
            s.append_view(&(ch as *mut HtmlText).value)
        }
    }
    const v = s.view()
    return builder.allocate_view(&v)
}

// Parses `("name" [, isolate] [, group = "g"]) { <fixture/> <script>steps</script> }`.
// The caller has already consumed the `#universal_test` token.
func ut_parse_body(parser : *mut Parser, builder : *mut ASTBuilder, statement : bool) : *mut UniversalTestDecl {

    if(!parser.increment_if(ChemicalTokenType.LParen as int)) {
        parser.error("expected '(' after #universal_test");
        return null;
    }

    var name = std::string_view()
    const nt = parser.getToken()
    if(nt.type == ChemicalTokenType.String as int || nt.type == ChemicalTokenType.BacktickString as int) {
        name = builder.allocate_view(&nt.value)
        parser.increment()
    } else {
        parser.error("expected a test name string");
    }

    var isolate = false
    var group = std::string_view()
    while(parser.increment_if(ChemicalTokenType.CommaSym as int)) {
        const ot = parser.getToken()
        if(ot.type != ChemicalTokenType.Identifier as int) {
            parser.error("expected an option identifier");
            break;
        }
        if(ot.value.equals(std::string_view("isolate"))) {
            isolate = true
            parser.increment()
        } else if(ot.value.equals(std::string_view("group"))) {
            parser.increment()
            if(!parser.increment_if(ChemicalTokenType.EqualSym as int)) {
                parser.error("expected '=' after 'group'");
            }
            const gt = parser.getToken()
            if(gt.type == ChemicalTokenType.String as int || gt.type == ChemicalTokenType.BacktickString as int) {
                group = builder.allocate_view(&gt.value)
                parser.increment()
            } else {
                parser.error("expected a group name string");
            }
        } else {
            parser.error("unknown #universal_test option");
            parser.increment()
        }
    }

    if(!parser.increment_if(ChemicalTokenType.RParen as int)) {
        parser.error("expected ')' after #universal_test options");
    }

    if(!parser.increment_if(ChemicalTokenType.LBrace as int)) {
        parser.error("expected '{' to start the test body");
        return null;
    }

    const root = parseHtmlRoot(parser, builder)

    if(!parser.increment_if(TokenType.RBrace as int)) {
        parser.error("expected '}' to end the test body");
    }

    // separate the single <script> (raw steps) from the fixture
    var steps = std::string_view()
    var i : size_t = 0
    while(i < root.children.size()) {
        const ch = root.children.get(i)
        var remove = false
        if(ch.kind == HtmlChildKind.Element) {
            const el = ch as *mut HtmlElement
            if(el.name.equals(std::string_view("script"))) {
                steps = ut_collect_script_text(el, builder)
                remove = true
            }
        }
        if(remove) {
            root.children.erase(i)
        } else {
            i++
        }
    }

    const decl = builder.allocate<UniversalTestDecl>()
    new (decl) UniversalTestDecl {
        name : name,
        group : group,
        isolate : isolate,
        statement : statement,
        params_ready : false,
        fixture : root as *mut HtmlRoot,
        steps : steps,
        fixtureFn : null
    }
    return decl
}

@no_mangle
public func ut_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    const loc = parser.getEncodedLocation(parser.getToken())
    const decl = ut_parse_body(parser, builder, false)
    if(decl == null) { return null }

    // create the fixture renderer now, so the collected metadata can reference it
    const voidType = builder.make_void_type(loc)
    const fnName = ut_sanitize_name(decl.name, builder)
    const funcDecl = builder.make_function(&fnName, voidType, false, parser.getParentNode(), loc)
    funcDecl.add_body()
    decl.fixtureFn = funcDecl

    const utMacroName = std::string_view("universal_test")
    const nodes_arr : []*mut ASTNode = []
    const node = builder.make_top_level_embedded_node(spec, &utMacroName, decl, ut_known_type, ut_child_res, ut_cross_mod, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(decl.fixture.dyn_values.data(), decl.fixture.dyn_values.size()), parser.getParentNode(), loc)

    // register the test for discovery (args: name, isolate, group, steps, fixture_fn)
    const controller = parser.getAnnotationController()
    const def = controller.getDefinition(std::string_view("universal_test"))
    if(def != null) {
        var args = std::vector<*mut Value>()
        args.push(builder.make_string_value(&decl.name, loc) as *mut Value)
        args.push(builder.make_bool_value(decl.isolate, loc) as *mut Value)
        args.push(builder.make_string_value(&decl.group, loc) as *mut Value)
        args.push(builder.make_string_value(&decl.steps, loc) as *mut Value)
        const fnId = builder.make_identifier(&fnName, funcDecl as *mut ASTNode, false, loc)
        args.push(fnId as *mut Value)
        controller.collect(node, def, std::span<*mut Value>(args.data(), args.size()))
    }

    return node
}

@no_mangle
public func ut_parseMacroNodeStatement(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ASTNode {
    const loc = parser.getEncodedLocation(parser.getToken())
    const decl = ut_parse_body(parser, builder, true)
    if(decl == null) { return null }
    const utMacroName = std::string_view("universal_test")
    const nodes_arr : []*mut ASTNode = []
    return builder.make_embedded_node(AccessSpecifier.Internal, &utMacroName, decl, ut_known_type, ut_child_res, std::span<*mut ASTNode>(nodes_arr), std::span<*mut Value>(decl.fixture.dyn_values.data(), decl.fixture.dyn_values.size()), parser.getParentNode(), loc)
}

@no_mangle
public func ut_symResSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
    // signature resolution happens in ut_symResNode
}

@no_mangle
public func ut_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const root = node.getDataPtr() as *mut UniversalTestDecl
    if(root.statement) { return }
    var builder = resolver.getJobBuilder()
    const fnName = ut_sanitize_name(root.name, &raw mut builder)
    resolver.declare_tld_default(&fnName, node)
}

@no_mangle
public func ut_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const root = node.getDataPtr() as *mut UniversalTestDecl
    const resolver = visitor.getSymbolResolver()
    const table = visitor.getSymbolTable()
    const diagnoser = visitor.getAstDiagnoser()
    const loc = node.getEncodedLocation()

    if(root.statement) {
        // the enclosing function already has `page` in scope
        visitor.visitEmbeddedNode(node)
        sym_res_root(root.fixture as *mut HtmlRoot, visitor, loc)
        return
    }

    var builder = resolver.getJobBuilder()

    const htmlPageNode = resolver.resolve(std::string_view("HtmlPage"))
    if(htmlPageNode == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
    }

    const funcDecl = root.fixtureFn
    if(funcDecl == null) {
        resolver.error(std::string_view("universal test fixture function was not created"), loc);
        return
    }

    if(!root.params_ready) {
        const linked = builder.make_linked_type(std::string_view("HtmlPage"), htmlPageNode, loc)
        const ref = builder.make_reference_type(linked, true, loc)
        const param = builder.make_function_param(std::string_view("page"), ref, 0, null, false, funcDecl, loc)
        funcDecl.get_params().push(param)
        root.params_ready = true
    }
    const param = funcDecl.get_params().get(0)

    // resolve the fixture with `page` in scope, so sym_res_root finds it and the
    // components, exactly like #html
    table.scope_start()
    table.declare(std::string_view("page"), param)
    visitor.visitEmbeddedNode(node)
    sym_res_root(root.fixture as *mut HtmlRoot, visitor, loc)
    table.scope_end()
}

// Emits the fixture container + production SSR + the registered steps JS into
// `vec` (a function body or a statement scope).
func ut_emit_test(
    builder : *mut ASTBuilder,
    diagnoser : *mut ASTDiagnoser,
    root : *mut UniversalTestDecl,
    vec : *mut VecRef<ASTNode>,
    parent : *mut ASTNode,
    fallback_loc : ubigint
) {
    var converter = ASTConverter {
        builder : builder,
        diagnoser : diagnoser,
        fallback_loc : fallback_loc,
        support : &raw mut root.fixture.support,
        vec : vec,
        parent : parent,
        str : std::string()
    }

    // fixture container: every test owns its own <div data-ut="..."> so many
    // tests can share one page without colliding
    var open = std::string("<div data-ut=\"")
    open.append_view(&root.name)
    open.append_view("\">")
    converter.emit_append_html_from_str(&mut open)

    // fixture: the production SSR/hydration pipeline, verbatim
    converter.convertHtmlRoot(root.fixture as *mut HtmlRoot)

    var close = std::string("</div>")
    converter.emit_append_html_from_str(&mut close)

    // the test's raw JS steps, registered into the page bundle
    var js = std::string()
    js.append_view("window.__ut_register(\"")
    js.append_view(&root.name)
    js.append_view("\", ")
    if(root.isolate) {
        js.append_view("true")
    } else {
        js.append_view("false")
    }
    // No parameter: the harness exposes `t` as a global, so a test may declare
    // its own `const t = ...` without colliding with a parameter name.
    js.append_view(", async function(){\n")
    js.append_view(&root.steps)
    js.append_view("\n});\n")
    converter.emit_append_js_from_str(&mut js)
}

@no_mangle
public func ut_replacementNodeDeclare(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut UniversalTestDecl
    if(root.statement) { return value }
    return root.fixtureFn
}

@no_mangle
public func ut_replacementNode(builder : *mut ASTBuilder, diagnoser : *mut ASTDiagnoser, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut UniversalTestDecl
    const loc = intrinsics::get_raw_location()

    if(root.statement) {
        var scope = builder.make_scope(value.getParent(), loc)
        ut_emit_test(builder, diagnoser, root, scope.getNodes(), value.getParent(), value.getEncodedLocation())
        return scope
    }

    const funcDecl = root.fixtureFn
    var body = funcDecl.add_body()
    ut_emit_test(builder, diagnoser, root, body, funcDecl, value.getEncodedLocation())

    var scope = builder.make_scope(value.getParent(), loc)
    scope.getNodes().push(funcDecl)
    return scope
}
