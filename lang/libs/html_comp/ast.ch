public struct ComponentParam {
    var name : std::string_view
    var is_optional : bool
}

public enum MountStrategy {
    Default,
    Preact,
    React,
    Solid,
    Universal
}

public enum TemplateTokenKind {
    Text,
    PropAccess,
    ChemicalValue,
    Children,
    NestedComponent,
    Spread,
    MergedAttribute
}

public enum MergedAttrSegmentKind {
    Text,
    PropAccess,
    ChemicalValue,
    SpreadAttr
}

public struct MergedAttrSegment {
    var kind : MergedAttrSegmentKind
    var value : std::string_view
    var chemicalValue : *mut Value = null
}

public struct MergedAttribute {
    var name : std::string_view
    var segments : std::vector<MergedAttrSegment>
}

public struct TemplateToken {
    var kind : TemplateTokenKind
    var value : std::string_view
    var chemicalValue : *mut Value = null
    var jsxElement : *mut void = null
    var mergedAttr : *mut MergedAttribute = null
}

public struct ComponentSignature {
    var name : std::string_view
    var propsName : std::string_view
    var params : std::vector<ComponentParam>
    var functionNode : *mut FunctionDeclaration = null
    var mountStrategy : MountStrategy = MountStrategy.Default
    var access : AccessSpecifier = AccessSpecifier.Private
    var universalTemplate : std::vector<TemplateToken>
    var universalInit : std::string_view
}

public func get_module_scoped_name(functionNode : *mut ASTNode, name : std::string_view, str : &mut std::string) {
    if(functionNode != null) {
        const modScope = functionNode.getModScope();
        if(modScope != null) {
            const scopeName = modScope.getScopeName();
            if(!scopeName.empty()) {
                str.append_view(scopeName);
                str.append('_');
            }
            str.append_view(modScope.getModuleName());
            str.append('_');
        }
    }
    str.append_view(name);
}
