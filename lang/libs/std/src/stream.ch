impl core::stream::Stream for CommandLineStream {

    // `spec == 0` is the plain path (one `fwrite` per value, no temporary).
    // A formatted value is rendered by `StringStream` and written once, so the
    // console and `append_expr` can never disagree about formatting.

    func writeChar(&self, value : char, spec : u64) {
        if(spec == 0u64) {
            unsafe { fwrite(&raw value, 1, 1, get_stdout()) };
            return
        }
        var buf = std::string()
        var sink = std::StringStream { str : &mut buf }
        sink.writeChar(value, spec)
        self.write_buffer(&buf)
    }

    func writeUChar(&self, value : uchar, spec : u64) {
        if(spec == 0u64) {
            unsafe { fwrite(&raw value, 1, 1, get_stdout()) };
            return
        }
        var buf = std::string()
        var sink = std::StringStream { str : &mut buf }
        sink.writeUChar(value, spec)
        self.write_buffer(&buf)
    }

    func writeSigned(&self, value : bigint, spec : u64) {
        if(spec == 0u64) {
            var buf : [64]char;
            var len = self.i64_to_chars(&raw mut buf[0], value);
            unsafe { fwrite(&raw buf[0], 1, len, get_stdout()) };
            return
        }
        var out = std::string()
        var sink = std::StringStream { str : &mut out }
        sink.writeSigned(value, spec)
        self.write_buffer(&out)
    }

    func writeUnsigned(&self, value : ubigint, spec : u64) {
        if(spec == 0u64) {
            var buf : [64]char;
            var len = self.u64_to_chars(&raw mut buf[0], value);
            unsafe { fwrite(&raw buf[0], 1, len, get_stdout()) };
            return
        }
        var out = std::string()
        var sink = std::StringStream { str : &mut out }
        sink.writeUnsigned(value, spec)
        self.write_buffer(&out)
    }

    func writeBool(&self, value : bool, spec : u64) {
        if(spec == 0u64) {
            if(value) {
                unsafe { fwrite("true" as *void, 1, 4, get_stdout()) };
            } else {
                unsafe { fwrite("false" as *void, 1, 5, get_stdout()) };
            }
            return
        }
        var buf = std::string()
        var sink = std::StringStream { str : &mut buf }
        sink.writeBool(value, spec)
        self.write_buffer(&buf)
    }

    unsafe func writeStr(&self, value : *char, length : ubigint, spec : u64) {
        if(spec == 0u64) {
            unsafe { fwrite(value, 1, length, get_stdout()) };
            return
        }
        var buf = std::string()
        var sink = std::StringStream { str : &mut buf }
        unsafe { sink.writeStr(value, length, spec) }
        self.write_buffer(&buf)
    }

    func writeFloat(&self, value : float, spec : u64) {
        // one formatter: delegate to `std::string`'s rendering instead of
        // keeping a second, divergent conversion here
        var s = std::string()
        if(spec == 0u64) {
            s.append_float(value, -1)
        } else {
            s.append_double_spec(value as double, std::decode_format_spec(spec))
        }
        self.write_buffer(&s)
    }

    func writeDouble(&self, value : double, spec : u64) {
        var s = std::string()
        if(spec == 0u64) {
            s.append_double(value, -1)
        } else {
            s.append_double_spec(value, std::decode_format_spec(spec))
        }
        self.write_buffer(&s)
    }
}

public struct CommandLineStream {

    // write an already-rendered buffer as one write
    func write_buffer(&self, buf : &std::string) {
        unsafe { fwrite(buf.data(), 1, buf.size(), get_stdout()) }
    }

    // --- helpers --------------------------------------------------------

    // convert unsigned 64-bit to ASCII in out_buf, return length written
    internal func u64_to_chars(&self, out_buf : *mut char, value : ubigint) : size_t {
        unsafe {
            // fast path
            if(value == 0) {
                out_buf[0] = '0';
                return 1;
            }

            // temporary reversed digits (max 20 digits for 64-bit)
            var rev : [20]char;
            var ri : int = 0;
            while(value != 0) {
                const d = (value % 10) as uint;
                rev[ri] = ('0' as int + d as int) as char;
                ri = ri + 1;
                value = value / 10;
            }

            // reverse into out_buf
            var i : size_t = 0;
            while(i < (ri as size_t)) {
                out_buf[i] = rev[(ri - 1 - (i as int))];
                i = i + 1;
            }
            return ri as size_t;
    
        }
    }

    // convert signed 64-bit to ASCII in out_buf, return length written
    // handles INT64_MIN safely using the -(value+1) trick
    internal func i64_to_chars(&self, out_buf : *mut char, value : bigint) : size_t {
        unsafe {
            var pos : size_t = 0;
            if(value < 0) {
                out_buf[0] = '-';
                pos = 1;
                // handle INT64_MIN safely
                var tmp = value + 1; // still negative or zero
                var uv : ubigint = 0;
                if(tmp < 0) {
                    uv = (-(tmp)) as ubigint;
                    uv = uv + 1;
                } else {
                    // value was -1 -> tmp == 0 -> uv = 1
                    uv = 1;
                }
                // write uv after the '-'
                var written = self.u64_to_chars(out_buf + 1, uv);
                return pos + written;
            } else {
                // positive
                var w = self.u64_to_chars(out_buf, (value as ubigint));
                return w;
            }
    
        }
    }

}

// --- console output ------------------------------------------------------
//
// Every console function renders into a `std::string` first, through the same
// `StringStream` path `string.append_expr` uses, and then performs a single
// `fwrite`. Doing it this way means:
//
//   * one rendering implementation for every sink, so `println` and
//     `append_expr` can never disagree about how a value is formatted;
//   * a line is one write, so two threads cannot interleave inside a line;
//   * the write happens exactly where the statement is, and is flushed
//     immediately, so a long-running process shows its output.

@expand
public comptime func print(expr : %expressive_string) {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr)
    write_stdout(&line)
}

@expand
public comptime func println(expr : %expressive_string) {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr, '\n')
    write_stdout(&line)
}

@expand
public comptime func eprint(expr : %expressive_string) {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr)
    write_stderr(&line)
}

@expand
public comptime func eprintln(expr : %expressive_string) {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr, '\n')
    write_stderr(&line)
}

// Checked forms: `print`/`println` are best effort (a console write failure is
// reported by the return of these, not by the plain functions).
@expand
public comptime func try_print(expr : %expressive_string) : std::Result<std::Unit, std::io::IoError> {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr)
    return write_stdout(&line)
}

@expand
public comptime func try_println(expr : %expressive_string) : std::Result<std::Unit, std::io::IoError> {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr, '\n')
    return write_stdout(&line)
}

@expand
public comptime func try_eprint(expr : %expressive_string) : std::Result<std::Unit, std::io::IoError> {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr)
    return write_stderr(&line)
}

@expand
public comptime func try_eprintln(expr : %expressive_string) : std::Result<std::Unit, std::io::IoError> {
    var line = std::string()
    intrinsics::expr_str_block_value(std::StringStream { str : &mut line }, expr, '\n')
    return write_stderr(&line)
}

/// Write `line` to standard output as one write, then flush.
@retained
public func write_stdout(line : &std::string) : std::Result<std::Unit, std::io::IoError> {
    return write_console_line(get_stdout(), line)
}

/// Write `line` to standard error as one write, then flush.
@retained
public func write_stderr(line : &std::string) : std::Result<std::Unit, std::io::IoError> {
    return write_console_line(get_stderr(), line)
}

@retained
func write_console_line(file : *mut FILE, line : &std::string) : std::Result<std::Unit, std::io::IoError> {
    unsafe {
        const data = line.data()
        const len = line.size()
        var written : size_t = 0
        comptime if(def.windows) {
            // On Windows `get_stdout`/`get_stderr` open a second CRT stream over
            // the process handle, so `printf`'s buffer and ours are different
            // files. Flush the CRT around our write, or the two streams reach
            // the handle in the wrong order and our bytes only appear at exit.
            fflush(null)
            written = fwrite(data, 1, len, file)
            fflush(null)
        } else {
            // POSIX hands back the real `stdout`/`stderr`, so a single buffer is
            // shared with `printf` and ordering is automatic.
            written = fwrite(data, 1, len, file)
            fflush(file)
        }
        if(written < len) {
            // a short write means the console/pipe is broken or full; the caller
            // can tell the difference instead of losing bytes silently
            return std::Result.Err(std::io::error_from_errno(get_errno()))
        }
        return std::Result.Ok(std::Unit { })
    }
}