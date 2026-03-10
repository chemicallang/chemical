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

public struct ComponentSignature {
    var name : std::string_view
    var propsName : std::string_view
    var params : std::vector<ComponentParam>
    var functionNode : *mut FunctionDeclaration = null
    var mountStrategy : MountStrategy = MountStrategy.Default
    var universalTemplate : std::string_view
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
