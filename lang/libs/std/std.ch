@export
public func printf(format : *char, _ : any...) : int

if(def.win64) {
    @export
    public type size_t = ubigint
} else if(def.win32) {
    @export
    public type size_t = ulong
} else {
    @export
    public type size_t = ulong
}

@export
public func snprintf(buffer : *mut char, bufsz : size_t, format : *char, _ : any...) : int

@export
public func malloc(size : size_t) : *mut void

@export
public func realloc(block : *mut any, size : size_t) : *mut void

@export
public func free(block : *mut any)

@export
public func memcpy(
    _Dst : *mut any,
    _Src : *any,
    _Size : size_t
) : *mut void;

@export
public func strlen(
    _Str : *char
) : size_t;

@export
public func strcmp(str1 : *char, str2 : *char) : int;

@export
public func strncmp(str1 : *char, str2 : *char, n : size_t) : int

@export
public func strncpy(dest : *mut char, src : *char, count : size_t) : *mut char

@export
public func tolower(ch : int) : int

@export
public func toupper(ch : int) : int

@export
public func memcmp(ptr1 : *any, ptr2 : *any, num : size_t) : int

@export
public func memmove(dest : *mut void, src : *void, count : size_t) : *mut void

@export
public func memset(dest : *mut void, ch : int, count : size_t) : *mut void

/**
public func exit(code : int)
public func quick_exit(code : int)
public func abort()
public void perror(error_message : *char)
**/