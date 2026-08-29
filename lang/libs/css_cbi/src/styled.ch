// #styled macro support — reusable, cross-module styled components.
//
// A styled component is declared at the top level as:
//   #styled Name(root.tag) { color: red; }
//   #styled Name.div { color: red; }          (shorthand for the above)
//   #styled Name(OtherComp) { color: red; }   (wraps a component)
//
// It generates a real SSR function `Name(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void`
// that emits `<tag class="hash">children</tag>` (or forwards to the wrapped component) and injects the
// generated CSS rules into the page. It is usable from both #html and #universal JSX as `<Name ...>children</Name>`.

public struct StyledComponent {
    var signature : ComponentSignature
    var tag : std::string_view
    var isComponentWrap : bool
    var cssom : *mut CSSOM
    var innerFunctionNode : *mut ASTNode = null
    var pageParam : *mut ASTNode = null
    var attrsParam : *mut ASTNode = null
    var attrListRefType : *mut BaseType = null
    var attrListBaseType : *mut BaseType = null
    var childrenParam : *mut ASTNode = null
    var ssrTextDataFn : *mut ASTNode = null
    var ssrTextSizeFn : *mut ASTNode = null
    var appendHtmlFn : *mut ASTNode = null
    var renderHtmlAttrsFn : *mut ASTNode = null
    var renderHtmlAttrsWithBaseFn : *mut ASTNode = null
    var ssrTextNode : *mut ASTNode = null
}

public func styled_node_known_type_func(value : *EmbeddedNode) : *BaseType {
    return null;
}

public func styled_node_child_res_func(value : *EmbeddedNode, name : &std::string_view) : *ASTNode {
    return null;
}

public func styled_cross_mod_proxy_fn(obj : *mut void, node : *mut EmbeddedNode, fn : CrossModuleSymbolDeclarerFn, at_least_spec : AccessSpecifier) {
    const comp = node.getDataPtr() as *mut StyledComponent;
    if (comp.signature.access == AccessSpecifier.Public) {
        fn(obj, &comp.signature.name, node)
    }
}

@no_mangle
public func styled_initializeLexer(lexer : *mut Lexer) {
    // styled components reuse the css lexer for their body
    css_initializeLexer(lexer)
}

@no_mangle
public func styled_parseMacroNode(parser : *mut Parser, builder : *mut ASTBuilder, spec : AccessSpecifier) : *mut ASTNode {
    const tok = parser.getToken()
    const loc = parser.getEncodedLocation(tok)

    // 1. component name (PropertyName in the css lexer at declaration state)
    if(parser.getToken().type != TokenType.PropertyName as int) {
        parser.error("expected a styled component name")
        return null
    }
    var name = builder.allocate_view(&parser.getToken().value)
    parser.increment()

    var tag = std::string_view()
    var isComponentWrap = false

    const t = parser.getToken().type
    if(t == TokenType.LParen as int) {
        parser.increment()
        const inner = parser.getToken()
        if(inner.type == TokenType.DoubleQuotedValue as int) {
            // tag string — strip the surrounding double quotes
            var raw = inner.value
            if(raw.size() >= 2) {
                tag = builder.allocate_view(std::string_view(raw.data() + 1, raw.size() - 2))
            } else {
                tag = builder.allocate_view(std::string_view("div"))
            }
            parser.increment()
        } else if(inner.type == TokenType.PropertyName as int) {
            // wrap an existing component
            isComponentWrap = true
            tag = builder.allocate_view(&inner.value)
            parser.increment()
        } else {
            parser.error("expected a tag string or a component name inside (...)")
            return null
        }
        if(!parser.increment_if(TokenType.RParen as int)) {
            parser.error("expected ')'")
            return null
        }
    } else if(t == TokenType.ClassName as int) {
        // shorthand: #styled Name.div { ... }
        tag = builder.allocate_view(&parser.getToken().value)
        parser.increment()
    } else {
        parser.error("expected (tag) or .tag after the styled component name")
        return null
    }

    // 2. parse the css body
    if(!parser.increment_if(TokenType.LBrace as int)) {
        parser.error("expected '{' to start the styled component body")
        return null
    }
    var cssom : *mut CSSOM = parseCSSOM(parser, builder) as *mut CSSOM
    if(!parser.increment_if(TokenType.RBrace as int)) {
        parser.error("expected '}' to end the styled component body")
        return null
    }

    var comp = builder.allocate<StyledComponent>()
    new (comp) StyledComponent {
        signature : ComponentSignature {
            name : name,
            propsName : std::string_view(),
            params : std::vector<ComponentParam>(),
            functionNode : null,
            mountStrategy : MountStrategy.Styled,
            access : spec,
            rootNodeCount : 0,
            className : std::string_view()
        },
        tag : tag,
        isComponentWrap : isComponentWrap,
        cssom : cssom,
        innerFunctionNode : null,
        pageParam : null,
        attrsParam : null,
        attrListRefType : null,
        attrListBaseType : null,
        childrenParam : null,
        ssrTextDataFn : null,
        ssrTextSizeFn : null,
        appendHtmlFn : null,
        renderHtmlAttrsFn : null,
        renderHtmlAttrsWithBaseFn : null,
        ssrTextNode : null
    }

    const nodes_arr : []*mut ASTNode = []
    const node = builder.make_top_level_embedded_node(
        spec,
        std::string_view("styled"),
        comp,
        styled_node_known_type_func,
        styled_node_child_res_func,
        styled_cross_mod_proxy_fn,
        std::span<*mut ASTNode>(nodes_arr),
        std::span<*mut Value>(cssom.dyn_values.data(), cssom.dyn_values.size()),
        parser.getParentNode(),
        loc
    )

    // mark as a component so both #html and #universal can resolve it
    const controller = parser.getAnnotationController();
    const definition = controller.getDefinition("component");
    if(definition != null) {
        const args : []*mut Value = []
        controller.mark(node, definition, std::span<*mut Value>(args));
    }

    return node
}

@no_mangle
public func styled_symResDeclareNode(resolver : *mut SymbolResolver, node : *mut EmbeddedNode) {
    const comp = node.getDataPtr() as *mut StyledComponent;
    resolver.declare_tld_default(&comp.signature.name, node);
}

@no_mangle
public func styled_symResSigNode(visitor : *mut SymResLinkSignature, node : *mut EmbeddedNode) {
    // nothing to resolve at link-signature time
}

@no_mangle
public func styled_symResNode(visitor : *mut SymResLinkBody, node : *mut EmbeddedNode) {
    const resolver = visitor.getSymbolResolver();
    const table = visitor.getSymbolTable();
    const diagnoser = visitor.getAstDiagnoser();
    const loc = node.getEncodedLocation();
    const root = node.getDataPtr() as *mut StyledComponent;

    const htmlPageType = resolver.resolve(std::string_view("HtmlPage"));
    if(htmlPageType == null) {
        resolver.error(std::string_view("could not find HtmlPage"), loc);
        return;
    }

    var builder = resolver.getJobBuilder();

    // create the SSR function: Name(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void
    const voidType = builder.make_void_type(loc);
    const funcDecl = builder.make_function(&root.signature.name, voidType, false, node.getParent(), loc);
    if(root.signature.access == AccessSpecifier.Public || root.signature.access == AccessSpecifier.Protected) {
        funcDecl.setAccessSpecifier(root.signature.access)
    }

    const linked = builder.make_linked_type(std::string_view("HtmlPage"), htmlPageType, loc);
    const ref = builder.make_reference_type(linked, true, loc);
    const params = funcDecl.get_params();
    const pageParam = builder.make_function_param(std::string_view("page"), ref, 0, null, false, funcDecl, loc);
    params.push(pageParam);
    root.pageParam = pageParam;

    // attrs parameter (the component's attributes, including any user-provided class)
    const attrListNodeType = builder.make_linked_type(std::string_view("SsrAttributeList"), resolver.resolve(std::string_view("SsrAttributeList")), loc);
    const attrListPtrType = builder.make_ptr_type(attrListNodeType as *mut BaseType, false, loc);
    const attrsParam = builder.make_function_param(std::string_view("attrs"), attrListPtrType as *mut BaseType, 1, null, false, funcDecl, loc);
    params.push(attrsParam);
    root.attrsParam = attrsParam;
    root.attrListRefType = attrListPtrType as *mut BaseType;
    root.attrListBaseType = attrListNodeType as *mut BaseType;

    // children parameter (already-rendered children as an SsrText buffer slice)
    const childrenType = builder.make_linked_type(std::string_view("SsrText"), resolver.resolve(std::string_view("SsrText")), loc);
    const childrenParam = builder.make_function_param(std::string_view("children"), childrenType as *mut BaseType, 2, null, false, funcDecl, loc);
    params.push(childrenParam);
    root.childrenParam = childrenParam;

    // add a body (mirrors universal)
    funcDecl.add_body();

    // resolve css support (page + css emitter functions) into the cssom support
    var support = &raw mut root.cssom.support
    support.pageNode = pageParam;
    support.appendCssCharFn = htmlPageType.child("append_css_char");
    support.appendCssCharPtrFn = htmlPageType.child("append_css_char_ptr");
    support.appendCssFn = htmlPageType.child("append_css");
    support.appendCssIntFn = htmlPageType.child("append_css_integer");
    support.appendCssUIntFn = htmlPageType.child("append_css_uinteger");
    support.appendCssFloatFn = htmlPageType.child("append_css_float");
    support.appendCssDoubleFn = htmlPageType.child("append_css_double");
    support.requireCssHashFn = htmlPageType.child("require_css_hash");
    support.setCssHashFn = htmlPageType.child("set_css_hash");
    support.requireRandomCssHashFn = htmlPageType.child("require_random_css_hash");
    support.setRandomCssHashFn = htmlPageType.child("set_random_css_hash");

    if(support.appendCssFn == null || support.requireCssHashFn == null) {
        resolver.error(std::string_view("css emitter functions are required on 'page' for styled components to work"), loc);
        return;
    }

    // resolve html emitter functions used by the SSR body
    root.appendHtmlFn = htmlPageType.child("append_html");
    if(root.appendHtmlFn == null) {
        resolver.error(std::string_view("'append_html' is required on 'page' for styled components to work"), loc);
        return;
    }
    root.renderHtmlAttrsFn = resolver.resolve(std::string_view("renderHtmlAttrs"));
    if(root.renderHtmlAttrsFn == null) {
        resolver.error(std::string_view("'renderHtmlAttrs' is required for styled components to work"), loc);
        return;
    }
    root.renderHtmlAttrsWithBaseFn = resolver.resolve(std::string_view("renderHtmlAttrsWithBase"));
    if(root.renderHtmlAttrsWithBaseFn == null) {
        resolver.error(std::string_view("'renderHtmlAttrsWithBase' is required for styled components to work"), loc);
        return;
    }
    root.ssrTextNode = resolver.resolve(std::string_view("SsrText"));
    if(root.ssrTextNode == null) {
        resolver.error(std::string_view("'SsrText' is required for styled components to work"), loc);
        return;
    }
    root.ssrTextDataFn = childrenParam.child("data");
    root.ssrTextSizeFn = childrenParam.child("size");

    // if wrapping a component, resolve the inner component function
    if(root.isComponentWrap) {
        const inner = resolver.resolve(&root.tag);
        if(inner == null) {
            resolver.error(std::string_view("could not find the component to wrap for the styled component"), loc);
            return;
        }
        const innerEmbedded = inner as *mut EmbeddedNode;
        const innerComp = innerEmbedded.getDataPtr() as *mut StyledComponent;
        root.innerFunctionNode = innerComp.signature.functionNode;
    }

    root.signature.functionNode = funcDecl;

    table.scope_start();
    table.declare(std::string_view("page"), pageParam);
    visitor.visitEmbeddedNode(node);
    table.scope_end();
}

@no_mangle
public func styled_replacementNodeDeclare(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut StyledComponent;
    return root.signature.functionNode;
}

func styled_make_html_call(builder : *mut ASTBuilder, pageParam : *mut ASTNode, fnNode : *mut ASTNode, text : std::string_view, parent : *mut ASTNode, loc : ubigint) : *mut FunctionCallNode {
    var base = builder.make_identifier(std::string_view("page"), pageParam, false, loc);
    var id = builder.make_identifier(std::string_view("append_html"), fnNode, false, loc);
    var chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc);
    var call = builder.make_function_call_node(chain, parent, loc);
    call.get_args().push(builder.make_string_value(&text, loc));
    call.get_args().push(builder.make_ubigint_value(text.size(), loc));
    return call;
}

func styled_make_render_attrs_call(builder : *mut ASTBuilder, fnNode : *mut ASTNode, pageParam : *mut ASTNode, attrsId : *mut Value, baseClass : *mut Value, parent : *mut ASTNode, loc : ubigint) : *mut FunctionCallNode {
    var id = builder.make_identifier(std::string_view("renderHtmlAttrsWithBase"), fnNode, false, loc);
    var call = builder.make_function_call_node(id, parent, loc);
    var pageId = builder.make_identifier(std::string_view("page"), pageParam, false, loc);
    call.get_args().push(pageId as *mut Value);
    call.get_args().push(attrsId);
    call.get_args().push(baseClass);
    return call;
}

func styled_make_children_call(builder : *mut ASTBuilder, pageParam : *mut ASTNode, fnNode : *mut ASTNode, childrenParam : *mut ASTNode, dataFn : *mut ASTNode, sizeFn : *mut ASTNode, parent : *mut ASTNode, loc : ubigint) : *mut FunctionCallNode {
    var base = builder.make_identifier(std::string_view("page"), pageParam, false, loc);
    var id = builder.make_identifier(std::string_view("append_html"), fnNode, false, loc);
    var chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), loc);
    var call = builder.make_function_call_node(chain, parent, loc);

    var childrenId = builder.make_identifier(std::string_view("children"), childrenParam, false, loc);
    var dataId = builder.make_identifier(std::string_view("data"), dataFn, false, loc);
    var dataAccess = builder.make_access_chain(&std::span<*mut Value>([ childrenId, dataId ]), loc);
    call.get_args().push(dataAccess);

    var childrenId2 = builder.make_identifier(std::string_view("children"), childrenParam, false, loc);
    var sizeId = builder.make_identifier(std::string_view("size"), sizeFn, false, loc);
    var sizeAccess = builder.make_access_chain(&std::span<*mut Value>([ childrenId2, sizeId ]), loc);
    call.get_args().push(sizeAccess);
    return call;
}

@no_mangle
public func styled_replacementNode(builder : *mut ASTBuilder, value : *mut EmbeddedNode) : *ASTNode {
    const root = value.getDataPtr() as *mut StyledComponent;
    const funcNode = root.signature.functionNode;
    const body = funcNode.add_body();
    const location = intrinsics::get_raw_location();

    // Emit the generated CSS rules into the page (sets root.cssom.className).
    var converter = ASTConverter {
        builder : builder,
        support : &raw mut root.cssom.support,
        vec : body,
        parent : value.getParent(),
        str : std::string()
    }
    converter.convertCSSOM(root.cssom);

    // For wrap components, publish the generated class name on the signature so that
    // html_cbi's `#html` conversion injects it as a `class` attribute on the attrs it
    // forwards into this component. The inner component then merges its own class with
    // this one via `renderHtmlAttrsWithBase`, so the element carries BOTH the inner
    // component's generated class and the wrapper's generated class.
    if(root.isComponentWrap) {
        root.signature.className = root.cssom.className;
    }

    // The generated class name (e.g. "rABC123") that the CSS rules target.
    var baseClassVal = builder.make_struct_value(root.ssrTextNode, location);
    baseClassVal.add_value(std::string_view("data"), builder.make_string_value(&root.cssom.className, location));
    baseClassVal.add_value(std::string_view("size"), builder.make_ubigint_value(root.cssom.className.size(), location));

    if(root.isComponentWrap) {
        var base = builder.make_identifier(&root.tag, root.innerFunctionNode, false, location);
        var call = builder.make_function_call_node(base as *mut Value, funcNode, location);
        call.get_args().push(builder.make_identifier(std::string_view("page"), root.pageParam, false, location) as *mut Value);
        call.get_args().push(builder.make_identifier(std::string_view("attrs"), root.attrsParam, false, location) as *mut Value);
        call.get_args().push(builder.make_identifier(std::string_view("children"), root.childrenParam, false, location) as *mut Value);
        body.push(call as *mut ASTNode);
    } else {
        var attrsParamId = builder.make_identifier(std::string_view("attrs"), root.attrsParam, false, location);
        var attrsDeref = builder.make_dereference_value(attrsParamId as *mut Value, root.attrListBaseType, location);

        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, std::string_view("<"), funcNode, location) as *mut ASTNode);
        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, root.tag, funcNode, location) as *mut ASTNode);
        body.push(styled_make_render_attrs_call(builder, root.renderHtmlAttrsWithBaseFn, root.pageParam, attrsDeref as *mut Value, baseClassVal, funcNode, location) as *mut ASTNode);
        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, std::string_view(">"), funcNode, location) as *mut ASTNode);
        body.push(styled_make_children_call(builder, root.pageParam, root.appendHtmlFn, root.childrenParam, root.ssrTextDataFn, root.ssrTextSizeFn, funcNode, location) as *mut ASTNode);
        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, std::string_view("</"), funcNode, location) as *mut ASTNode);
        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, root.tag, funcNode, location) as *mut ASTNode);
        body.push(styled_make_html_call(builder, root.pageParam, root.appendHtmlFn, std::string_view(">"), funcNode, location) as *mut ASTNode);
    }

    var scope = builder.make_scope(funcNode.getParent(), location);
    var scope_nodes = scope.getNodes();
    scope_nodes.push(funcNode);
    return scope;
}
