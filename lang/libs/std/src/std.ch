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

public type usize = size_t

if(def.win64) {
    @extern
    public type isize = bigint
} else if(def.win32) {
    @extern
    public type isize = long
} else {
    @extern
    public type isize = long
}

export printf;
export snprintf;
export malloc;
export realloc;
export free;
export strcmp;
export strncmp;
export strncpy;
export tolower;
export toupper;
export memcmp;
export memmove;
export memset;
export fflush;
export memcpy;
export strlen;

/**
public func exit(code : int)
public func quick_exit(code : int)
public void perror(error_message : *char)
**/