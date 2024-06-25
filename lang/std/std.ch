@cbi:global("compiler")
func printf(format : char*, _ : any...) : int

@cbi:global("compiler")
typealias size_t = ubigint

@cbi:global("compiler")
func malloc(size : size_t) : void*

@cbi:global("compiler")
func realloc(block : void*, size : size_t) : void*

@cbi:global("compiler")
func free(block : void*)

@cbi:global("compiler")
func memcpy(
    _Dst : void*,
    _Src : void*,
    _Size : size_t
) : void*;

@cbi:global("compiler")
func strlen(
    _Str : char*
) : size_t;