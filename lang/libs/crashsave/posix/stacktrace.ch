/* Helper: run addr2line and capture first line of output (file:line), return 0 on success */
func addr2line_for_exec(exe_path : *char, addr_offset_hex : *char, out_buf : *char, out_sz : size_t) : int {
    /* Build command: addr2line -e <exe_path> -f -p <offset>  (we use -f -p to get function+file:line)
       We'll use popen to read output. */
    char [4096]cmd;
    /* Quote exe_path minimally (not robust against quotes). This is simple and workable. */
    snprintf(cmd, sizeof(cmd), "addr2line -e \"%s\" -f -p %s", exe_path, addr_offset_hex);
    var fp = popen(cmd, "r");
    if (!fp) return -1;
    if (fgets(out_buf, out_sz as int, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    /* trim newline */
    var len = strlen(out_buf);
    if (len && out_buf[len-1] == '\n') out_buf[len-1] = '\0';
    pclose(fp);
    return 0;
}

/* Get executable path into out[sz] (Linux /proc method) */
func get_exec_path(out : *mut char, sz : size_t) : int {
    var linkbuf : [64]char;
    snprintf(linkbuf, sizeof(linkbuf), "/proc/%d/exe", getpid() as int);
    var n = readlink(linkbuf, out, sz - 1);
    if (n < 0) return -1;
    if (n >= sz as ssize_t) n = sz - 1;
    out[n] = '\0';
    return 0;
}

/* Crash handler for Linux signals */
func handle_crash_linux(sig : int) {
    var bt : [256]*void;
    var size = backtrace(bt, 256);
    var exe_path : [4096]char;
    memset(exe_path, 0, sizeof(exe_path))

    get_exec_path(exe_path, sizeof(exe_path));

    printf("%s: Program crashed with signal %d\n", __FUNCTION__, sig);
    printf("Dumping backtrace.\n");

    var strings = backtrace_symbols(bt, size);
    if (!strings) {
        printf("-- no backtrace symbols --\n");
        return;
    }

    for (var i : int = 1; i < size; ++i) {
        /* backtrace_symbols output typically contains "binary(+0xoffset) [0xaddr]" or similar.
         * We will try to extract the +0xoffset part; fallback to printing the pointer.
         */
        var sym = strings[i];
        /* find '+' and ')' */
        var p_plus: *mut char = null;
        var p_paren : *mut char = null;
        var p = sym;
        while (*p != '\0') {
            if (*p == '+') { p_plus = p; break; }
            ++p;
        }
        p = sym;
        while (*p != '\0') {
            if (*p == ')') { p_paren = p; break; }
            ++p;
        }
        var offset_buf : [64]char;
        memset(offset_buf, 0, sizeof(offset_buf))
        var got_offset : int = 0;
        if (p_plus && p_paren && p_paren > p_plus) {
            var offlen = (p_paren - p_plus - 1) as size_t;
            if (offlen < sizeof(offset_buf)) {
                memcpy(offset_buf, p_plus + 1, offlen);
                offset_buf[offlen] = '\0';
                got_offset = 1;
            }
        }
        if (got_offset) {
            var resolved : [1024]char;
            memset(resolved, 0, sizeof(resolved))
            if (addr2line_for_exec(exe_path, offset_buf, resolved, sizeof(resolved)) == 0) {
                printf("[%d] %s\n", i, resolved);
            } else {
                /* fallback to raw symbol */
                printf("[%d] %s\n", i, sym);
            }
        } else {
            printf("[%d] %s\n", i, sym);
        }
    }

    free(strings);
    printf("-- END OF BACKTRACE --\n");
    /* abort to let OS handle the rest */
    abort();
}

/* Install / disable utilities for Linux */
public func install_crash_handler() {
    signal(SIGSEGV, handle_crash_linux);
    signal(SIGFPE, handle_crash_linux);
    signal(SIGILL, handle_crash_linux);
}

func uninstall_crash_handler() {
    signal(SIGSEGV, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
}