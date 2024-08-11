func printf(format : char*, _ : any...) : int

func sprintf(to : char*, format : char*, _ : any...) : int

@not_in_c
typealias size_t = ubigint

func malloc(size : size_t) : void*

func realloc(block : void*, size : size_t) : void*

func free(block : void*)

func memcpy(
    _Dst : void*,
    _Src : void*,
    _Size : size_t
) : void*;

func strlen(
    _Str : char*
) : size_t;

func strcmp (str1 : char*, str2 : char*) : int;

func strncmp(str1 : char*, str2 : char*, n : size_t) : int

func memcmp(ptr1 : void*, ptr2 : void*, num : size_t) : int

/**
func exit(code : int)
func quick_exit(code : int)
func abort()
void perror(error_message : char*)
**/