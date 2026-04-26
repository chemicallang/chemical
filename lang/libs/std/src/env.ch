public namespace std {

/**
 * Searches for an environmental variable with name name in the host-specified environment list
 * and returns the value as a std::string.
 */
public func get_env(name : &std::string_view) : std::Option<std::string> {
    var ptr = getenv(name.data());
    if(ptr == null) {
        return std::Option.None<std::string>();
    }
    return std::Option.Some<std::string>(std::string.make_no_len(ptr));
}

}
