
func enable_terminal_colors() : int {
    if(def.windows) {
        // TODO: enable these
        // /* Try to enable VT processing on Windows 10+ so ANSI codes work */
        // HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        // if (hOut == INVALID_HANDLE_VALUE) return 0;
        // DWORD dwMode = 0;
        // if (!GetConsoleMode(hOut, &dwMode)) return 0;
        // dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        // if (!SetConsoleMode(hOut, dwMode)) return 0;
        // return 1;
    } else {
        return 1;
    }
}

/* If colors are disabled, these macros evaluate to empty strings. */
var colors_enabled : int = 0;

public comptime const ANSI_RESET =  "\x1b[0m"
public comptime const ANSI_BOLD =   "\x1b[1m"
public comptime const ANSI_RED =    "\x1b[31m"
public comptime const ANSI_GREEN =  "\x1b[32m"
public comptime const ANSI_YELLOW = "\x1b[33m"
public comptime const ANSI_BLUE =   "\x1b[34m"
public comptime const ANSI_MAGENTA = "\x1b[35m"
public comptime const ANSI_CYAN =   "\x1b[36m"
public comptime const ANSI_GRAY =   "\x1b[90m"

func col_reset() : *char {
    if(colors_enabled) return ANSI_RESET else return "";
}
func col_bold() : *char  {
    if(colors_enabled) return ANSI_BOLD else return "";
}
func col_red() : *char   {
    if(colors_enabled) return ANSI_RED else return "";
}
func col_green() : *char {
    if(colors_enabled) return ANSI_GREEN else return "";
}
func col_yellow() : *char{
    if(colors_enabled) return ANSI_YELLOW else return "";
}
func col_blue() : *char  {
    if(colors_enabled) return ANSI_BLUE else return "";
}
func col_mag() : *char   {
    if(colors_enabled) return ANSI_MAGENTA else return "";
}
func col_cyan() : *char  {
    if(colors_enabled) return ANSI_CYAN else return "";
}
func col_gray() : *char  {
    if(colors_enabled) return ANSI_GRAY else return "";
}

/* ---- Log type name / severity mapping ---- */

func log_type_name(t : LogType) : *char {
    switch (t) {
        LogType.Success => return "SUCCESS";
        LogType.UnknownFailure => return "UNKNOWN";
        LogType.ConfigFailure => return "CONFIG";
        LogType.IOFailure => return "I/O";
        LogType.NetworkFailure => return "NETWORK";
        LogType.MemoryFailure => return "MEMORY";
        LogType.RuntimeFailure => return "RUNTIME";
        LogType.OutOfBoundFailure => return "OUT_OF_BOUND";
        LogType.ResourceFailure => return "RESOURCE";
        LogType.TodoFailure => return "TODO";
        LogType.SecurityFailure => return "SECURITY";
        LogType.WTFFailure => return "WTF";
        default => return "UNKNOWN";
    }
}

/* Short icon for log severity */
func log_type_icon(t : LogType) : *char {
    switch (t) {
        LogType.Success => { return "✓"; }
        LogType.TodoFailure => { return "…"; }
        LogType.WTFFailure => { return "‼"; }
        LogType.SecurityFailure => { return "🔒"; }
        LogType.UnknownFailure => { return "✖"; }
        default => { return "!"; }
    }
}

/* Color by severity */
func log_type_color(t : LogType) : *char {
    switch (t) {
        LogType.Success => { return col_green(); }
        LogType.TodoFailure => { return col_yellow(); }
        LogType.UnknownFailure, LogType.WTFFailure, LogType.SecurityFailure, LogType.ConfigFailure,
        LogType.IOFailure, LogType.NetworkFailure, LogType.MemoryFailure, LogType.RuntimeFailure,
        LogType.OutOfBoundFailure, LogType.ResourceFailure => {
            return col_red();
        }
        default => { return col_reset(); }
    }
}

/* ---- Utility: safe string getters ---- */
func safe_str(s : *char) : *char {
    if(s) return s else return "(null)";
}

/* ---- Pretty-print function ---- */

func print_test_results(states : *TestFunctionState, count : size_t) {

    if (!states) {
        fprintf(get_stderr(), "print_test_results: states pointer is NULL\n");
        return;
    }

    colors_enabled = enable_terminal_colors();

    /* First pass: compute totals and collect groups */
    var total = count;
    var passed : size_t = 0
    var failed : size_t = 0;
    /* We'll build a small map of unique group names (simple array) */
    var groups : **char = null;
    var groups_cap : size_t = 0
    var groups_len : size_t = 0;

    for (var i : size_t = 0; i < count; ++i) {
        const s = &states[i];
        if (s.exitCode == 0) ++passed; else ++failed;

        var g = "(no-group)";
        if (s.fn && s.fn.group.data() && s.fn.group.data()[0]) g = s.fn.group.data();

        /* check if group already in list */
        var found : int = 0;
        for (var j : size_t = 0; j < groups_len; ++j) {
            if (strcmp(groups[j], g) == 0) { found = 1; break; }
        }
        if (!found) {
            if (groups_len + 1 > groups_cap) {
                var newcap : size_t
                if(groups_cap) {
                    newcap = groups_cap * 2
                } else {
                    newcap = 8
                };
                const tmp : **char = realloc(groups, newcap * sizeof(*char)) as **char;
                if (!tmp) { /* allocation failed: bail out */
                    fprintf(get_stderr(), "print_test_results: out of memory\n");
                    free(groups);
                    return;
                }
                groups = tmp;
                groups_cap = newcap;
            }
            groups[groups_len++] = g;
        }
    }

    var col_one : *char
    if(failed) {
        col_one = col_red()
    } else {
        col_one = col_gray()
    }

    /* Summary header */
    printf("%s%sTest run summary%s\n", col_bold(), col_cyan(), col_reset());
    printf("  Total: %zu | %sPassed: %s%zu%s | %sFailed: %s%zu%s\n\n",
           total,
           col_green(), col_green(), passed, col_reset(),
           col_one, col_red(), failed, col_reset());

    /* For each group, print its tests in order found */
    for (var gi : size_t = 0; gi < groups_len; ++gi) {
        const group_name = groups[gi];
        printf("%s%sGroup: %s%s\n", col_bold(), col_blue(), safe_str(group_name), col_reset());
        /* print under group: iterate states and print those that match */
        for (var i : size_t = 0; i < count; ++i) {
            const s = &states[i];
            var g = "(no-group)";
            var fn_name = "(unknown)";
            if (s.fn) {
                if (s.fn.group.data() && s.fn.group.data()[0]) g = s.fn.group.data();
                if (s.fn.name.data() && s.fn.name.data()[0]) fn_name = s.fn.name.data();
            }
            if (strcmp(g, group_name) != 0) continue;

            /* Test header line */
            const status_color = if(s.exitCode == 0) col_green() else col_red();
            const status_text = if(s.exitCode == 0) "PASS" else "FAIL";
            printf("  %s- %s%s%s%s", status_color, col_bold(), fn_name, col_reset(), col_reset());
            printf("  [%s%u%s] ", status_color, s.exitCode as uint, col_reset());
            printf("%s%s%s\n", status_color, status_text, col_reset());

            /* Print logs, if any */
            if (s.logs.size() > 0) {
                for (var li : size_t = 0; li < s.logs.size(); ++li) {
                    const log = s.logs.get_ptr(li);
                    const lt_color = log_type_color(log.type);
                    const icon = log_type_icon(log.type);
                    const lname = log_type_name(log.type);
                    /* compose location string if present */
                    if (log.line > 0 || log.character > 0) {
                        printf("     %s%s%s %s%s (line %zu:%zu)%s\n",
                               lt_color, icon, col_reset(),
                               lt_color, lname,
                               log.line, log.character,
                               col_reset());
                    } else {
                        printf("     %s%s%s %s%s%s\n",
                               lt_color, icon, col_reset(),
                               lt_color, lname, col_reset());
                    }
                    /* message may be multiline; indent each line */
                    if (!log.message.empty()) {
                        const msg = log.message.data();
                        /* print each line separately */
                        const start = msg;
                        while (*start !in '\0') {
                            const nl = strchr(start, '\n');
                            var len : size_t
                            if(nl) {
                                len = (nl - start) as size_t
                            } else {
                                len = strlen(start);
                            };
                            /* print with indentation */
                            printf("       %s", col_gray());
                            fwrite(start, 1, len, get_stdout());
                            printf("%s\n", col_reset());
                            if (!nl) break;
                            start = nl + 1;
                        }
                    }
                }
            } else {
                /* no logs */
                printf("     %s(no logs)%s\n", col_gray(), col_reset());
            }
            /* Horizontal separator between tests */
            printf("\n");
        }
    }

    var some_col : *char
    if(failed) {
        some_col = col_red()
    } else {
        some_col = col_gray()
    }

    /* final summary line */
    printf("%sSummary: %zu tests — %s%zu passed%s, %s%zu failed%s\n\n",
           col_bold(), total,
           col_green(), passed, col_reset(),
           some_col, failed, col_reset());

    free(groups);
}