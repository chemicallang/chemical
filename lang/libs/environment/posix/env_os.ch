// environment — POSIX platform implementations.

public namespace environment {

    @extern
    func setenv(name : *char, value : *char, overwrite : int) : int

    @extern
    func unsetenv(name : *char) : int

    @extern
    func getcwd(buf : *mut char, size : size_t) : *mut char

    @extern
    func chdir(path : *char) : int

} // end namespace environment