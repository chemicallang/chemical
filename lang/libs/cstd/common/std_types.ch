/**
 * ptrdiff_t is the signed integer type of the result of subtracting two pointers.
 * @see https://en.cppreference.com/w/c/types/ptrdiff_t
 */
if(def.win64) {
    @extern
public type ptrdiff_t = bigint
} else if(def.win32) {
    @extern
public type ptrdiff_t = long
} else {
    @extern
public type ptrdiff_t = long
}

/**
 * size_t is the unsigned integer type of the result of sizeof, offsetof and _Alignof(until C23)alignof(since C23), depending on the data model.
 * @see https://en.cppreference.com/w/c/types/size_t
 */
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

/**
 * max_align_t is a type whose alignment requirement is at least as strict (as large) as that of every scalar type.
 * @see https://en.cppreference.com/w/c/types/max_align_t
 */
@extern
public type max_align_t = double

/**
 * nullptr_t is the type of the predefined null pointer constant, nullptr. It is a distinct type that is not itself a pointer type. It can be implicitly converted to any pointer type or bool, and the result is the null pointer value of that type or false respectively. No type other than nullptr_t itself can be converted or explicitly cast to nullptr_t.
 * @see https://en.cppreference.com/w/c/types/nullptr_t
 */
@extern
public type nullptr_t = *mut void

/**
 * a typedef for the same type as size_t, used to self-document functions that range-check their parameters at runtime
 * @see https://en.cppreference.com/w/c/error
 */
@extern
public type rsize_t = size_t;

/**
 * a typedef for the type int, used to self-document functions that return errno values
 * @see https://en.cppreference.com/w/c/error
 */
@extern
public type errno_t = int;