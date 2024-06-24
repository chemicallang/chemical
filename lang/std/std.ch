func printf(format : char*, _ : any...) : int

func malloc(size : size_t) : void*

func realloc(block : void*, size : size_t) : void*

func free(block : void*)

typealias size_t = ubigint

func memcpy(
    _Dst : void*,
    _Src : void*,
    _Size : size_t
) : void*;

/**

func exit(code : int)

func quick_exit(code : int)

func abort()

void perror(error_message : char*)
**/