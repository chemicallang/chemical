// environment — Windows platform implementations.

public namespace environment {

    @extern
    func SetEnvironmentVariableA(name : *char, value : *char) : int

} // end namespace environment