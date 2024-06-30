@cbi:global("compiler")
@dispose(false)
func printf(format : char*, _ : any...) : int

@cbi:global("compiler")
@dispose(false)
typealias size_t = ubigint

@cbi:global("compiler")
@dispose(false)
func malloc(size : size_t) : void*

@cbi:global("compiler")
@dispose(false)
func realloc(block : void*, size : size_t) : void*

@cbi:global("compiler")
@dispose(false)
func free(block : void*)

@cbi:global("compiler")
@dispose(false)
func memcpy(
    _Dst : void*,
    _Src : void*,
    _Size : size_t
) : void*;

@cbi:global("compiler")
@dispose(false)
func strlen(
    _Str : char*
) : size_t;

@cbi:global("compiler")
@dispose(false)
func strcmp (str1 : char*, str2 : char*) : int;

/**
func exit(code : int)
func quick_exit(code : int)
func abort()
void perror(error_message : char*)
**/