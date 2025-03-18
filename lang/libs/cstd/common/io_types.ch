/**
 * FILE object is implementation defined
 * Each FILE object denotes a C stream.
 * C standard does not specify whether FILE is a complete object type. While it may be possible to copy a valid FILE, using a pointer to such a copy as an argument for an I/O function invokes unspecified behavior. In other words, FILE may be semantically non-copyable.
 * I/O streams can be used for both unformatted and formatted input and output. Furthermore, the functions that handle input and output can also be locale-sensitive, such that wide/multibyte conversions are performed as necessary.
 * @see https://en.cppreference.com/w/c/io/FILE
 */
@no_init
public struct FILE {

}