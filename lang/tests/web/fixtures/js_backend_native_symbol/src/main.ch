// JS backend — native-symbol fixture (expected to FAIL to compile for JS).
//
// `getenv` is a real C function declared `@extern` in cstd and available on the
// native backend. A client/shared module must not depend on it, and the backend
// has to say exactly that — "defined in another Chemical module" is a different,
// wrong explanation.

public func main() : int {
    unsafe {
        var home = getenv("HOME")
        if(home != null) {
            printf("home=%s\n", home)
        } else {
            printf("no_home\n")
        }
    }
    return 0
}
