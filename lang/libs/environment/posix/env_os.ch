// environment — POSIX platform implementations.

public namespace environment {

    @extern
    func setenv(name : *char, value : *char, overwrite : int) : int

    @extern
    func unsetenv(name : *char) : int

} // end namespace environment