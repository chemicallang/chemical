// Async wrappers for the environment library (design §7 Tier 4).
//
// Additive: the synchronous API is unchanged. Environment access does not block
// on I/O, so these exist only for API symmetry and for callers that want a
// uniform awaitable surface.
public namespace environment {

public async func get_async(name : std::string_view) : std::Option<std::string> {
    return get(name)
}

public async func set_async(name_ : std::string_view, value : std::string_view) : std::Result<UnitTy, EnvError> {
    return set(name_, value)
}

public async func unset_async(name : std::string_view) : std::Result<UnitTy, EnvError> {
    return unset(name)
}

}
