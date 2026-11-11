// environment — cross-platform environment variable and process-argument access.

public namespace environment {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using path::Path;
using path::PathBuf;

// POSIX: the environ variable (declared in env_os.ch on Windows)
comptime if(!def.windows) {
    // Use get_environ() rather than a direct `environ`/`__environ` import:
    // in a non-PIE ELF the public `environ` symbol gets a copy relocation
    // that leaves it NULL, while get_environ() returns the real pointer.
}

func env_name(name : &str) : Result<std::CString, EnvError> {
    var made = std::cstring_make(name)
    if(made is Result.Err) {
        return Result.Err(EnvError.InvalidInput())
    }
    var Ok(owned) = made else unsafe { unreachable }
    return Result.Ok(owned)
}

func env_value(value : &str) : Result<std::CString, EnvError> {
    return env_name(value)
}

func pathbuf_from(s : &string) : PathBuf {
    return PathBuf.from(std::str.make_view(s.to_view()))
}

// ---------------------------------------------------------------------------
// Environment variable access
// ---------------------------------------------------------------------------

/// Get an environment variable by name.
public func var(name : &str) : Result<string, EnvError> {
    var made = env_name(name)
    if(made is Result.Err) {
        var Err(e) = made else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(cname) = made else unsafe { unreachable }
    comptime if(def.windows) {
        var needed = unsafe(GetEnvironmentVariableA(unsafe(cname.as_ptr()), null, 0))
        if(needed == 0) {
            return Result.Err(EnvError.NotFound())
        }
        var buf : [4096]char
        var got = unsafe(GetEnvironmentVariableA(unsafe(cname.as_ptr()), &raw mut buf[0], 4096))
        if(got == 0 || got >= 4096) {
            return Result.Err(EnvError.NotFound())
        }
        return Result.Ok(unsafe(string.make_no_len(&raw mut buf[0])))
    } else {
        var ptr = unsafe(getenv(unsafe(cname.as_ptr())))
        if(ptr == null) {
            return Result.Err(EnvError.NotFound())
        }
        return Result.Ok(unsafe(string.make_no_len(ptr)))
    }
}

/// Get an environment variable as an OS-native string.
public func var_os(name : &str) : Option<std::OsString> {
    var r = var(name)
    if(r is Result.Err) {
        return Option.None<std::OsString>()
    }
    var Ok(value) = r else unsafe { unreachable }
    return Option.Some<std::OsString>(std::OsString(&value))
}

/// Set an environment variable. Process-global mutation races with other
/// threads; use `Command::env` for child processes.
public unsafe func set_var(name : &str, value : &str) : Result<UnitTy, EnvError> {
    var n = env_name(name)
    if(n is Result.Err) { var Err(e) = n else unsafe { unreachable }; return Result.Err(e) }
    var v = env_value(value)
    if(v is Result.Err) { var Err(e) = v else unsafe { unreachable }; return Result.Err(e) }
    var Ok(cname) = n else unsafe { unreachable }
    var Ok(cvalue) = v else unsafe { unreachable }
    comptime if(def.windows) {
        var r = unsafe(SetEnvironmentVariableA(unsafe(cname.as_ptr()), unsafe(cvalue.as_ptr())))
        if(r == 0) { return Result.Err(EnvError.OperationFailed(string("SetEnvironmentVariableA failed"))) }
        return Result.Ok(UnitTy{})
    } else {
        var r = unsafe(setenv(unsafe(cname.as_ptr()), unsafe(cvalue.as_ptr()), 1))
        if(r != 0) { return Result.Err(EnvError.OperationFailed(string("setenv failed"))) }
        return Result.Ok(UnitTy{})
    }
}

/// Remove an environment variable.
public unsafe func remove_var(name : &str) : Result<UnitTy, EnvError> {
    var n = env_name(name)
    if(n is Result.Err) { var Err(e) = n else unsafe { unreachable }; return Result.Err(e) }
    var Ok(cname) = n else unsafe { unreachable }
    comptime if(def.windows) {
        var r = unsafe(SetEnvironmentVariableA(unsafe(cname.as_ptr()), null))
        if(r == 0) { return Result.Err(EnvError.OperationFailed(string("SetEnvironmentVariableA failed"))) }
        return Result.Ok(UnitTy{})
    } else {
        var r = unsafe(unsetenv(unsafe(cname.as_ptr())))
        if(r != 0) { return Result.Err(EnvError.OperationFailed(string("unsetenv failed"))) }
        return Result.Ok(UnitTy{})
    }
}

// ---------------------------------------------------------------------------
// Common environment variables
// ---------------------------------------------------------------------------

/// Get the PATH (or Path on Windows) environment variable.
public func path_env() : Option<string> {
    comptime if(def.windows) {
        return get("Path")
    } else {
        return get("PATH")
    }
}

/// Get the HOME (or USERPROFILE on Windows) directory.
public func home_dir() : Option<string> {
    comptime if(def.windows) {
        return get("USERPROFILE")
    } else {
        return get("HOME")
    }
}

/// Get the current user name.
public func user_name() : Option<string> {
    comptime if(def.windows) {
        return get("USERNAME")
    } else {
        return get("USER")
    }
}

/// Get the current working directory.
public func current_dir() : Result<PathBuf, EnvError> {
    comptime if(def.windows) {
        var buf : [4096]char
        var len = unsafe(GetCurrentDirectoryA(4096, &raw mut buf[0]))
        if(len == 0 || len >= 4096) {
            return Result.Err(EnvError.OperationFailed(string("GetCurrentDirectoryA failed")))
        }
        var s = unsafe(string.make_no_len(&raw mut buf[0]))
        return Result.Ok(pathbuf_from(&s))
    } else {
        var buf : [4096]char
        var r = unsafe(getcwd(&raw mut buf[0], 4096 as size_t))
        if(r == null) {
            return Result.Err(EnvError.OperationFailed(string("getcwd failed")))
        }
        var s = unsafe(string.make_no_len(&raw mut buf[0]))
        return Result.Ok(pathbuf_from(&s))
    }
}

/// Change the process working directory. Process-global mutation races with
/// other threads; prefer `Command::current_dir` for children.
public unsafe func set_current_dir(dir : &Path) : Result<UnitTy, EnvError> {
    var native = dir.native_ptr()
    if(native == null) {
        return Result.Err(EnvError.InvalidInput())
    }
    comptime if(def.windows) {
        var r = unsafe(SetCurrentDirectoryW(native))
        if(r == 0) { return Result.Err(EnvError.OperationFailed(string("SetCurrentDirectoryW failed"))) }
        return Result.Ok(UnitTy{})
    } else {
        var r = unsafe(chdir(native))
        if(r != 0) { return Result.Err(EnvError.OperationFailed(string("chdir failed"))) }
        return Result.Ok(UnitTy{})
    }
}

/// Enumerate all environment variables as "KEY=VALUE" strings.
public unsafe func all() : vector<string> {
        unsafe {
        var result = vector<string>()
        comptime if(def.windows) {
            var env_ptr = GetEnvironmentStringsA()
            if(env_ptr == null) { return result }
            var p = env_ptr as *char
            while(true) {
                if(p[0] as char == '\0' as char && p[1] as char == '\0' as char) { break }
                var entry = string.make_no_len(p)
                var k : size_t = 0
                while(p[k] != 0) { k += 1 }
                p = (p as *u8 + k + 1) as *char
                result.push(entry)
            }
            FreeEnvironmentStringsA(env_ptr)
        } else {
            var ep = get_environ()
            if(ep == null) { return result }
            var i : int = 0;
            while(ep[i] != null) {
                result.push(unsafe(string.make_no_len(ep[i] as *char)))
                i += 1;
            }
        }
        return result

        }
    }

/// Get the temporary directory path.
public func temp_dir() : PathBuf {
    comptime if(def.windows) {
        var t = var("TEMP")
        if(t is Result.Err) {
            t = var("TMP")
        }
        if(t is Result.Ok) {
            var Ok(value) = t else unsafe { unreachable }
            return pathbuf_from(&value)
        }
        return PathBuf.from(".")
    } else {
        var t = var("TMPDIR")
        if(t is Result.Ok) {
            var Ok(value) = t else unsafe { unreachable }
            return pathbuf_from(&value)
        }
        return PathBuf.from("/tmp")
    }
}

/// Get the shell being used.
public func shell() : Option<string> {
    comptime if(def.windows) {
        return get("ComSpec")
    } else {
        return get("SHELL")
    }
}

/// Get the terminal type.
public func term() : Option<string> {
    return get("TERM")
}

// ---------------------------------------------------------------------------
// Process arguments
// ---------------------------------------------------------------------------

func push_arg(out : *mut vector<string>, view : string_view) : Result<UnitTy, EnvError> {
    var made = std::try_from_utf8(view)
    if(made is Result.Err) {
        return Result.Err(EnvError.InvalidInput())
    }
    var Ok(text) = made else unsafe { unreachable }
    if(!unsafe(out.try_push(string(text.to_view())))) {
        return Result.Err(EnvError.OutOfMemory())
    }
    return Result.Ok(UnitTy{})
}

/// Process command-line arguments, including argv[0].
public func args() : Result<vector<string>, EnvError> {
    var out = vector<string>()
    comptime if(def.windows) {
        var cmd = unsafe(GetCommandLineA())
        if(cmd == null) {
            return Result.Ok(out)
        }
        var len = unsafe(strlen(cmd))
        var view = unsafe(string_view(cmd, len))
        // Simple whitespace split; quoted arguments are preserved verbatim.
        var start : size_t = 0
        var i : size_t = 0
        while(i <= len) {
            if(i == len || view.at(i) == ' ' || view.at(i) == '\t') {
                if(i > start) {
                    // `subview(start, end)` takes an absolute end offset, not a
                    // length: passing `i - start` made later arguments ask for a
                    // range that ends before it starts (an out-of-bounds panic).
                    var r = push_arg(&raw mut out, view.subview(start, i))
                    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e) }
                }
                start = i + 1
            }
            i += 1
        }
        return Result.Ok(out)
    } else {
        var f = unsafe(fopen("/proc/self/cmdline", "rb"))
        if(f == null) {
            return Result.Err(EnvError.OperationFailed(string("cannot read process arguments")))
        }
        var bytes = vector<u8>()
        var chunk : [4096]u8
        while(true) {
            var n = unsafe(fread(&raw mut chunk[0], 1 as size_t, 4096 as size_t, f))
            if(n == 0) { break }
            var i : size_t = 0
            while(i < n) {
                if(!bytes.try_push(chunk[i])) {
                    var _ = unsafe(fclose(f))
                    return Result.Err(EnvError.OutOfMemory())
                }
                i += 1
            }
            if(n < 4096) { break }
        }
        var _ = unsafe(fclose(f))
        var start : size_t = 0
        var i : size_t = 0
        while(i < bytes.size()) {
            if(bytes.at(i) == 0) {
                if(i > start) {
                    var view = unsafe(string_view(bytes.data() as *char + start, i - start))
                    var r = push_arg(&raw mut out, view)
                    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e) }
                }
                start = i + 1
            }
            i += 1
        }
        if(start < bytes.size()) {
            var view = unsafe(string_view(bytes.data() as *char + start, bytes.size() - start))
            var r = push_arg(&raw mut out, view)
            if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e) }
        }
        return Result.Ok(out)
    }
}

// ---------------------------------------------------------------------------
// Compatibility aliases
// ---------------------------------------------------------------------------

/// Compatibility alias for `var`, returning `Option`.
public func get(name : &str) : Option<string> {
    var r = var(name)
    if(r is Result.Err) { return Option.None<string>() }
    var Ok(value) = r else unsafe { unreachable }
    return Option.Some<string>(value)
}

/// Compatibility alias: value or default.
public func get_or(name : &str, default_val : string_view) : string {
    var opt = get(name)
    if(opt is Option.Some) {
        var Some(v) = opt else unsafe { unreachable }
        return v
    }
    return string(default_val)
}

/// Compatibility alias for `set_var`.
public unsafe func set(name : &str, value : &str) : Result<UnitTy, EnvError> {
    unsafe { return set_var(name, value) }
}

/// Compatibility alias for `remove_var`.
public unsafe func unset(name : &str) : Result<UnitTy, EnvError> {
    unsafe { return remove_var(name) }
}

// ---------------------------------------------------------------------------
// Unit type
// ---------------------------------------------------------------------------

public struct UnitTy {}

} // end namespace environment
