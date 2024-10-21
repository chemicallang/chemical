/**
 * TODO ptrdiff_t's implementation is unknown
 * @see https://en.cppreference.com/w/c/types/ptrdiff_t
 */
public typealias ptrdiff_t = ubigint

/**
 * TODO size_t's implementation is unknown
 * @see https://en.cppreference.com/w/c/types/size_t
 */
public typealias size_t = ubigint

/**
 * TODO max_align_t's implementation is unknown
 * @see https://en.cppreference.com/w/c/types/max_align_t
 */
public typealias max_align_t = ubigint

/**
 * TODO nullptr_t's implementation is unknown
 * @see https://en.cppreference.com/w/c/types/nullptr_t
 */
public typealias nullptr_t = ubigint

/**
 * a typedef for the same type as size_t, used to self-document functions that range-check their parameters at runtime
 * @see https://en.cppreference.com/w/c/error
 */
public typealias rsize_t = size_t;

/**
 * a typedef for the type int, used to self-document functions that return errno values
 * @see https://en.cppreference.com/w/c/error
 */
public typealias errno_t = int;