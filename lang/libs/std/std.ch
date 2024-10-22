public func printf(format : *char, _ : any...) : int

public func sprintf(to : *char, format : *char, _ : any...) : int

if(compiler::is_clang()) {
    public typealias size_t = ubigint
} else {
    @comptime
    public typealias size_t = ubigint
}

public func malloc(size : size_t) : *void

public func realloc(block : *any, size : size_t) : *void

public func free(block : *any)

public func memcpy(
    _Dst : *any,
    _Src : *any,
    _Size : size_t
) : *void;

public func strlen(
    _Str : *char
) : size_t;

public func strcmp (str1 : *char, str2 : *char) : int;

public func strncmp(str1 : *char, str2 : *char, n : size_t) : int

public func memcmp(ptr1 : *any, ptr2 : *any, num : size_t) : int

/**
public func exit(code : int)
public func quick_exit(code : int)
public func abort()
public void perror(error_message : *char)
**/