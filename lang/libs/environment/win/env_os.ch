// environment — Windows platform implementations.

public namespace environment {

    @extern
    func SetEnvironmentVariableA(name : *char, value : *char) : int

    @extern
    func GetEnvironmentVariableA(name : *char, buf : *mut char, size : u32) : u32

    @extern
    func GetEnvironmentStringsA() : *u8

    @extern
    func FreeEnvironmentStringsA(pEnvBlock : *u8) : int

} // end namespace environment