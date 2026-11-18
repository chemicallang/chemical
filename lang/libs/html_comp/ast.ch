public struct ComponentParam {
    var name : std::string_view
    var is_optional : bool
}

public enum MountStrategy {
    Default,
    Preact,
    React,
    Solid,
    Universal,
    Styled
}

public struct ComponentSignature {
    var name : std::string_view
    var propsName : std::string_view
    var params : std::vector<ComponentParam>
    // Typed props (§3.15a). `#universal Card(props : CardProps)` names a struct;
    // the name is resolved to `propsStruct` at symres so call sites can validate
    // attribute names against the declared fields. Empty/null when a component
    // keeps untyped props (the pre-v1 form).
    var propsTypeName : std::string_view
    var propsStruct : *mut StructDefinition = null
    // Fields marked `@optional` in the props struct. They may be omitted, are never
    // defaulted, and are simply absent from the SSR attributes and the client payload,
    // so the body's `props.x != null` (or truthiness) test sees "not passed". Collected
    // at symres, where the annotation controller is reachable, and consumed by both
    // required-prop checks (the JSX path and the `#html` path).
    var optionalProps : std::vector<std::string_view>
    var functionNode : *mut FunctionDeclaration = null
    // The parsed JS/JSX body of a `#universal` component, back-pointed so the
    // router (a consumer) can validate `props.X` reads / `dangerouslySetInnerHTML`
    // (R11/R12). `*mut void` to avoid an html_comp -> js_syntax dependency;
    // consumers cast to `*mut JsNode`. Null for non-universal components.
    var js_body : *mut void = null
    // When a styled component wraps a universal component, the SSR call still
    // goes through the styled component's function (so its generated CSS is
    // emitted), but hydration must target the inner universal component (which
    // owns the client-side JS). `hydrateFunctionNode`/`hydrateName` carry that
    // inner target.
    var hydrateFunctionNode : *mut FunctionDeclaration = null
    var hydrateName : std::string_view
    var mountStrategy : MountStrategy = MountStrategy.Default
    var access : AccessSpecifier = AccessSpecifier.Private
    var rootNodeCount : uint = 0
    var className : std::string_view
}

public unsafe func get_module_scoped_name(functionNode : *mut ASTNode, name : std::string_view, str : &mut std::string) {
        unsafe {
        if(functionNode != null) {
            const modScope = functionNode.getModScope();
            if(modScope != null) {
                const scopeName = modScope.getScopeName();
                if(!scopeName.empty()) {
                    str.append_view(&scopeName);
                    str.append('_');
                }
                str.append_view(modScope.getModuleName());
                str.append('_');
            }
        }
        str.append_view(&name);

        }
    }
