/* unistd.h */
@extern
public func readlink(path : *char, buf : *mut char, bufsiz : size_t) : ssize_t

@extern
public func _exit(status : int) : void

var g_crashing : int = 0
var g_on_crash : () => void = null

/* Get executable path via /proc/self/exe */
func get_exec_path(out : *mut char, sz : size_t) : int {
    var n = readlink("/proc/self/exe", out, sz - 1);
    if (n < 0) return -1;
    if (n >= sz as ssize_t) n = (sz - 1) as ssize_t;
    out[n] = '\0';
    return 0;
}

/* Run addr2line with the given offset/address */
func addr2line_lookup(exe_path : *char, offset_str : *char, out_buf : *mut char, out_sz : size_t) : int {
    var cmd : [4096]char;
    snprintf(cmd, sizeof(cmd), "addr2line -e \"%s\" -f -C -p %s 2>/dev/null", exe_path, offset_str);
    var fp = popen(cmd, "r");
    if (!fp) return -1;
    var ret = fgets(out_buf, out_sz as int, fp);
    pclose(fp);
    if (ret == null) return -1;
    var len = strlen(out_buf);
    if (len > 0 && out_buf[len-1] == '\n') out_buf[len-1] = '\0';
    return 0;
}

/* Extract the offset from a backtrace_symbols line and resolve it.
   Format: "binary(func+0xOFFSET) [0xADDR]" or "binary(+0xOFFSET) [0xADDR]" or "binary() [0xADDR]"
   For PIE: offset inside () is the ELF offset (correct for addr2line).
   For non-PIE: fall back to address in []. */
func resolve_symbol(sym : *mut char, exe_path : *char, out_buf : *mut char, out_sz : size_t) : int {
    var p = sym;
    while (*p != '\0' && *p != '(') ++p;
    if (*p == '\0') return -1;
    var paren_end_mark = p;
    while (*paren_end_mark != '\0' && *paren_end_mark != ')') ++paren_end_mark;
    if (*paren_end_mark == '\0') return -1;
    /* look for "+0x" inside parens */
    p = p + 1;
    while (p < paren_end_mark) {
        if (*p == '+') {
            var next = p + 1;
            if (*next == '0' && (*(next+1) == 'x' || *(next+1) == 'X')) {
                /* found offset, extract it */
                var hex_str = next;
                var hex_end = hex_str;
                while (hex_end < paren_end_mark) ++hex_end;
                var hex_len = (hex_end - hex_str) as size_t;
                var hex_buf : [256]char;
                if (hex_len > 0 && hex_len < sizeof(hex_buf) as size_t) {
                    memcpy(hex_buf as *mut void, hex_str as *void, hex_len);
                    hex_buf[hex_len] = '\0';
                    return addr2line_lookup(exe_path, hex_buf, out_buf, out_sz);
                }
            }
        }
        ++p;
    }
    /* fallback: extract address from "[0x...]" */
    p = sym;
    while (*p != '\0' && *p != '[') ++p;
    if (*p == '\0') return -1;
    var addr_start = p + 1;
    p = addr_start;
    while (*p != '\0' && *p != ']') ++p;
    if (*p == '\0') return -1;
    var addr_len = (p - addr_start) as size_t;
    var addr_buf : [256]char;
    if (addr_len == 0 || addr_len >= sizeof(addr_buf) as size_t) return -1;
    memcpy(addr_buf as *mut void, addr_start as *void, addr_len);
    addr_buf[addr_len] = '\0';
    return addr2line_lookup(exe_path, addr_buf, out_buf, out_sz);
}

/* Signal handler */
func handle_crash_linux(sig : int) : void {
    if (g_crashing) {
        _exit(1);
    }
    g_crashing = 1;
    if (g_on_crash) {
        g_on_crash();
    }

    fprintf(stderr, "Program crashed with signal %d\n", sig);
    fprintf(stderr, "Dumping backtrace.\n");

    var bt : [256]*void;
    var size = backtrace(bt, 256);

    var strings = backtrace_symbols(bt, size);
    if (!strings) {
        fprintf(stderr, "-- no backtrace symbols --\n");
    } else {
        var exe_path : [4096]char;
        memset(exe_path as *mut void, 0, sizeof(exe_path));
        get_exec_path(exe_path, sizeof(exe_path));

        for (var i : int = 1; i < size; ++i) {
            var sym = strings[i];
            var resolved : [1024]char;
            memset(resolved as *mut void, 0, sizeof(resolved));
            if (exe_path[0] != '\0' && resolve_symbol(sym, exe_path, resolved, sizeof(resolved)) == 0) {
                fprintf(stderr, "  [%d] %s\n", i, resolved);
            } else {
                fprintf(stderr, "  [%d] %s\n", i, sym);
            }
        }
    }

    fprintf(stderr, "-- END OF BACKTRACE --\n");
    fflush(null);
    abort();
}

/* Install signal handlers */
public func install_crash_handler(exe_path : *char, onCrash : () => void) {
    g_on_crash = onCrash;
    signal(SIGSEGV, handle_crash_linux);
    signal(SIGFPE, handle_crash_linux);
    signal(SIGILL, handle_crash_linux);
}

func uninstall_crash_handler() {
}
