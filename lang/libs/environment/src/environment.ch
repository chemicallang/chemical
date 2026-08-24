// environment — cross-platform environment variable access.
// Provides a comprehensive API for reading and writing environment variables.

public namespace environment {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// POSIX: the environ variable (declared in env_os.ch on Windows)
comptime if(!def.windows) {
    @extern var environ : **char
}

// ---------------------------------------------------------------------------
// Environment variable access
// ---------------------------------------------------------------------------

/// Get an environment variable by name.
/// Returns Some(value) if found, None if not found.
public func get(name : string_view) : Option<string> {
    comptime if(def.windows) {
        // Use GetEnvironmentVariableA instead of getenv because getenv
        // caches the environment block and doesn't reflect SetEnvironmentVariableA
        // changes within the same process.
        var needed = GetEnvironmentVariableA(name.data(), null, 0)
        if(needed == 0) {
            return Option.None<string>()
        }
        unsafe var buf : [4096]char
        var got = GetEnvironmentVariableA(name.data(), &raw mut buf[0], 4096)
        if(got == 0 || got >= 4096) {
            return Option.None<string>()
        }
        return Option.Some(string.make_no_len(&raw mut buf[0]))
    } else {
        var ptr = getenv(name.data());
        if(ptr == null) {
            return Option.None<string>();
        }
        return Option.Some(string.make_no_len(ptr));
    }
}

/// Get an environment variable with a default value.
/// Returns the value if found, or the default if not.
public func get_or(name : string_view, default_val : string_view) : string {
    var opt = get(name);
    if(opt is Option.Some) {
        var Some(v) = opt else unreachable;
        return v;
    }
    return string(default_val);
}

/// Set an environment variable.
/// Returns Err if the operation fails.
public func set(name : string_view, value : string_view) : Result<UnitTy, EnvError> {
    comptime if(def.windows) {
        var r = SetEnvironmentVariableA(name.data(), value.data());
        if(r == 0) { return Result.Err(EnvError.OperationFailed(string("SetEnvironmentVariableA failed"))); }
        return Result.Ok(UnitTy{});
    } else {
        var r = setenv(name.data(), value.data(), 1);
        if(r != 0) { return Result.Err(EnvError.OperationFailed(string("setenv failed"))); }
        return Result.Ok(UnitTy{});
    }
}

/// Unset/remove an environment variable.
public func unset(name : string_view) : Result<UnitTy, EnvError> {
    comptime if(def.windows) {
        var r = SetEnvironmentVariableA(name.data(), null);
        if(r == 0) { return Result.Err(EnvError.OperationFailed(string("SetEnvironmentVariableA failed"))); }
        return Result.Ok(UnitTy{});
    } else {
        var r = unsetenv(name.data());
        if(r != 0) { return Result.Err(EnvError.OperationFailed(string("unsetenv failed"))); }
        return Result.Ok(UnitTy{});
    }
}

// ---------------------------------------------------------------------------
// Common environment variables
// ---------------------------------------------------------------------------

/// Get the PATH (or Path on Windows) environment variable.
public func path() : Option<string> {
    comptime if(def.windows) {
        return get("Path");
    } else {
        return get("PATH");
    }
}

/// Get the HOME (or USERPROFILE on Windows) directory.
public func home_dir() : Option<string> {
    comptime if(def.windows) {
        return get("USERPROFILE");
    } else {
        return get("HOME");
    }
}

/// Get the current user name.
public func user_name() : Option<string> {
    comptime if(def.windows) {
        return get("USERNAME");
    } else {
        return get("USER");
    }
}

/// Get the current working directory.
public func current_dir() : Option<string> {
    comptime if(def.windows) {
        unsafe var buf : [1024]char
        var len = GetCurrentDirectoryA(1024, &raw mut buf[0])
        if(len == 0 || len >= 1024) {
            return Option.None<string>()
        }
        return Option.Some(string.make_no_len(&raw mut buf[0]))
    } else {
        return get("PWD");
    }
}

/// Enumerate all environment variables.
/// Returns a vector of "KEY=VALUE" strings.
public func all() : vector<string> {
    var result = vector<string>()
    comptime if(def.windows) {
        var env_ptr = GetEnvironmentStringsA()
        if(env_ptr == null) { return result }
        var p = env_ptr as *char
        while(true) {
            // Each entry is null-terminated; the block ends with a double null
            if(p[0] as char == '\0' as char && p[1] as char == '\0' as char) { break }
            var entry = string.make_no_len(p)
            // Advance past this entry
            var k : size_t = 0
            while(p[k] != 0) { k += 1 }
            p = (p as *u8 + k + 1) as *char
            result.push(entry)
        }
        FreeEnvironmentStringsA(env_ptr)
    } else {
        var ep = environ
        if(ep == null) { return result }
        var i : int = 0
        while(ep[i] != null) {
            result.push(string.make_no_len(ep[i]))
            i += 1
        }
    }
    return result
}

/// Get the temporary directory path.
public func temp_dir() : Option<string> {
    comptime if(def.windows) {
        return get("TEMP");
    } else {
        return get("TMPDIR");
    }
}

/// Get the shell being used.
public func shell() : Option<string> {
    comptime if(def.windows) {
        return get("ComSpec");
    } else {
        return get("SHELL");
    }
}

/// Get the terminal type.
public func term() : Option<string> {
    return get("TERM");
}

// ---------------------------------------------------------------------------
// Unit type
// ---------------------------------------------------------------------------

public struct UnitTy {}

} // end namespace environment
