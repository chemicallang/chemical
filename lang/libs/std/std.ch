public func printf(format : *char, _ : any...) : int

if(compiler::is_clang()) {
    public type size_t = ubigint
} else {
    @comptime
    public type size_t = ubigint
}

public func snprintf(buffer : *mut char, bufsz : size_t, format : *char, _ : any...) : int

public func malloc(size : size_t) : *mut void

public func realloc(block : *mut any, size : size_t) : *mut void

public func free(block : *mut any)

public func memcpy(
    _Dst : *mut any,
    _Src : *any,
    _Size : size_t
) : *mut void;

public func strlen(
    _Str : *char
) : size_t;

public func strcmp(str1 : *char, str2 : *char) : int;

public func strncmp(str1 : *char, str2 : *char, n : size_t) : int

public func strncpy(dest : *mut char, src : *char, count : size_t) : *mut char

public func tolower(ch : int) : int

public func toupper(ch : int) : int

public func memcmp(ptr1 : *any, ptr2 : *any, num : size_t) : int

public func memmove(dest : *mut void, src : *void, count : size_t) : *mut void

public func memset(dest : *mut void, ch : int, count : size_t) : *mut void

/**
public func exit(code : int)
public func quick_exit(code : int)
public func abort()
public void perror(error_message : *char)
**/