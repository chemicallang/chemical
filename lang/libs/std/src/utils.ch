public namespace std {
/**
 * safer way to replace and move members, however impacts runtime performance
 */
public func <T> replace(value : &mut T, repl : T) : T {
    var temp : T
    memcpy(&mut temp, &value, sizeof(T))
    memcpy(&mut value, &repl, sizeof(T))
    intrinsics::forget(repl)
    return temp;
}
}