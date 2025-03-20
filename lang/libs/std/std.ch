@extern
public func printf(format : *char, _ : any...) : int

if(def.win64) {
    @extern
    public type size_t = ubigint
} else if(def.win32) {
    @extern
    public type size_t = ulong
} else {
    @extern
    public type size_t = ulong
}

@extern
public func snprintf(buffer : *mut char, bufsz : size_t, format : *char, _ : any...) : int

@extern
public func malloc(size : size_t) : *mut void

@extern
public func realloc(block : *mut any, size : size_t) : *mut void

@extern
public func free(block : *mut any)

@extern
public func memcpy(
    _Dst : *mut any,
    _Src : *any,
    _Size : size_t
) : *mut void;

@extern
public func strlen(
    _Str : *char
) : size_t;

@extern
public func strcmp(str1 : *char, str2 : *char) : int;

@extern
public func strncmp(str1 : *char, str2 : *char, n : size_t) : int

@extern
public func strncpy(dest : *mut char, src : *char, count : size_t) : *mut char

@extern
public func tolower(ch : int) : int

@extern
public func toupper(ch : int) : int

@extern
public func memcmp(ptr1 : *any, ptr2 : *any, num : size_t) : int

@extern
public func memmove(dest : *mut void, src : *void, count : size_t) : *mut void

@extern
public func memset(dest : *mut void, ch : int, count : size_t) : *mut void

/**
public func exit(code : int)
public func quick_exit(code : int)
public func abort()
public void perror(error_message : *char)
**/