enum MessageCommandType {
    None,
    Log,
    QuitGroup,
    QuitAll
}

func to_msg_cmd_type(str : *char) : MessageCommandType {
    if (!str) return MessageCommandType.None; // guard nullptr
    switch (fnv1_hash(str)) {
        comptime_fnv1_hash("log") => { return MessageCommandType.Log; }
        comptime_fnv1_hash("quit_group") => { return MessageCommandType.QuitGroup; }
        comptime_fnv1_hash("quit_all") => { return MessageCommandType.QuitAll; }
        default => { return MessageCommandType.None; }
    }
}

func read_msg_type(msg_ptr : *mut *char) : MessageCommandType {
    if (!msg_ptr) return MessageCommandType.None;      // guard **null
    var msg = *msg_ptr;
    if (!msg)      return MessageCommandType.None;     // guard *null

    const CMD_MAX = 120;
    var command_buffer : [CMD_MAX]char;
    var out = &mut command_buffer[0];
    var written : int = 0;

    while (true) {
        var c = *msg;
        if (c == '\0' || c == ',') {
            if (written < CMD_MAX) {
                *out = '\0';
            } else {
                // ensure termination even if we filled the buffer completely
                command_buffer[CMD_MAX-1] = '\0';
            }
            *msg_ptr = msg; // leave at delimiter or '\0'
            break;
        } else {
            if (written < CMD_MAX - 1) { // keep room for '\0'
                *out = c;
                out++;
                written++;
            }
            // Always advance source, even if we stopped writing, to not overflow.
            msg++;
        }
    }

    return to_msg_cmd_type(&command_buffer[0]);
}

/*
return codes:
  0 = success (out filled)
 -1 = bad arguments (null pointers)
  1 = no digits found (nothing converted)
  2 = trailing non-space characters after number
  3 = out of range for int (overflow/underflow)
*/
func parse_int_w_end(s : *char, out : *mut int, endPtrPtr : *mut *char) : int {
    if (!s || !out || !endPtrPtr) return -1;  // also guard endPtrPtr
    set_errno(0);

    // Call strtol with a local endptr, then publish it back to *endPtrPtr
    var local_end : *mut char = null;
    var val : long = strtol(s, &mut local_end, 10);

    if (local_end == s) { // no conversion
        *endPtrPtr = s;   // keep caller’s pointer unchanged logically
        return 1;
    }

    // Allow trailing whitespace only
    var scan = local_end;
    while (*scan != '\0' && isspace(*scan as int)) {
        scan++;
    }
    if (*scan != '\0') {
        *endPtrPtr = local_end;
        return 2;
    }

    // Range check
    if (get_errno() == ERANGE || val > INT_MAX || val < INT_MIN) {
        *endPtrPtr = local_end;
        return 3;
    }

    *out = val as int;
    *endPtrPtr = local_end;
    return 0;
}

func parse_int_from_str(pstr : *mut *char) : int {
    if (!pstr || !*pstr) {
        fprintf(get_stderr(), "parse_int_from_str: null pointer passed\n");
        return 0;
    }

    const s = *pstr;

    var endptr : *mut char = null;

    set_errno(0);
    const val = strtol(s, &endptr, 10);

    if (endptr == s) {
        // No digits were found
        fprintf(get_stderr(), "parse_int_from_str: no digits at '%s'\n", s);
        *pstr = endptr; // leave pointer unchanged
        return 0;
    }

    if (get_errno() == ERANGE || val > INT_MAX || val < INT_MIN) {
        fprintf(get_stderr(), "parse_int_from_str: value out of range at '%s'\n", s);
        *pstr = endptr;
        return 0;
    }

    // Move caller's pointer to endptr (either \0 or first non-digit)
    *pstr = endptr;

    return val as int;
}

func read_char(msg_ptr : *mut *char, c : char) : bool {
    if (!msg_ptr) return false;
    var msg = *msg_ptr;
    if (!msg) return false;

    if (*msg == c) {
        msg++;
        *msg_ptr = msg;
        return true;
    } else {
        return false;
    }
}

func read_str(msg_ptr : *mut *char, into : &mut std::string) {
    if (!msg_ptr) return;
    var msg = *msg_ptr;
    if (!msg) return;

    // Append until NUL; caller is responsible for delimitation (e.g., already consumed the comma).
    while (*msg != '\0') {
        into.append(*msg); // use a single-char append; adjust to your std::string API
        msg++;
    }
    *msg_ptr = msg;
}

func parseLog(msg_ptr : *mut *char, log : &mut TestLog) {
    if (!msg_ptr) return;

    if (!read_char(msg_ptr, ',')) { return; }
    log.type = parse_int_from_str(msg_ptr) as LogType;

    if (!read_char(msg_ptr, ',')) { return; }
    log.line = parse_int_from_str(msg_ptr) as ubigint;

    if (!read_char(msg_ptr, ',')) { return; }
    log.character = parse_int_from_str(msg_ptr) as ubigint;

    if (!read_char(msg_ptr, ',')) { return; }

    read_str(msg_ptr, log.message);
}

/*
Message format examples:
$log,0,10,20,this is a normal log message
$log,1,10,20,this is an error log message
$log,2,10,20,this is a warning log message
$log,3,10,20,this is a info log message
$log,4,10,20,this is a success log message
$quit_group
$quit_all
*/
func process_message(state : &mut TestFunctionState, msg : *char) {
    if (!msg)   return;

    // Must start with '$' to be one of ours
    if (!read_char(&mut msg, '$')) {
        return;
    }

    var msgType = read_msg_type(&mut msg);

    switch (msgType) {
        MessageCommandType.None, default => {
            return;
        }
        MessageCommandType.Log => {
            var l = TestLog();
            parseLog(&mut msg, l);
            if(l.type !in LogType.Information, LogType.Warning, LogType.Success) {
                state.has_failed = true;
                state.has_error_log = true;
            }
            state.logs.push(l);
        }
        MessageCommandType.QuitGroup => {
            // TODO: quit current group
        }
        MessageCommandType.QuitAll => {
            // TODO: quit all
        }
    }
}

/* return codes:
 *   0 = success (out filled)
 *  -1 = bad arguments (null pointers)
 *   1 = no digits found (nothing converted)
 *   2 = trailing non-space characters after number
 *   3 = out of range for int (overflow/underflow)
 */
func parse_int(s : *char, out : *mut int) : int {
    var end_ptr : *char
    return parse_int_w_end(s, out, &mut end_ptr)
}
