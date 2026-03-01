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
