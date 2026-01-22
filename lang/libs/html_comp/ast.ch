public struct ComponentParam {
    var name : std::string_view
    var is_optional : bool
}

public enum MountStrategy {
    Default,
    Preact,
    React,
    Solid
}

public struct ComponentSignature {
    var name : std::string_view
    var propsName : std::string_view
    var params : std::vector<ComponentParam>
    var functionNode : *mut FunctionDeclaration = null
    var mountStrategy : MountStrategy = MountStrategy.Default
}