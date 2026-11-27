// P0.1: `link c "file.c"` + @extern declarations.
@extern
public func ui_probe_c_add(a : int, b : int) : int

@extern
public func ui_probe_c_version() : int

public func main() : int {
    var sum : int = 0
    var version : int = 0
    unsafe {
        sum = ui_probe_c_add(40, 2)
        version = ui_probe_c_version()
    }
    if(sum == 42 && version == 7) {
        println("P0.1 link_c PASS")
        return 0
    }
    println("P0.1 link_c FAIL")
    return 1
}
