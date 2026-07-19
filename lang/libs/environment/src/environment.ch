// environment — cross-platform environment variable access.
// Provides a comprehensive API for reading and writing environment variables.

public namespace environment {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

// ---------------------------------------------------------------------------
// Environment variable access
// ---------------------------------------------------------------------------

/// Get an environment variable by name.
/// Returns Some(value) if found, None if not found.
public func get(name : string_view) : Option<string> {
    var ptr = getenv(name.data());
    if(ptr == null) {
        return Option.None<string>();
    }
    return Option.Some(string.make_no_len(ptr));
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
        if(r == 0) { return Result.Err(EnvError.OperationFailed("SetEnvironmentVariableA failed")); }
        return Result.Ok(UnitTy{});
    } else {
        var r = setenv(name.data(), value.data(), 1);
        if(r != 0) { return Result.Err(EnvError.OperationFailed("setenv failed")); }
        return Result.Ok(UnitTy{});
    }
}

/// Unset/remove an environment variable.
public func unset(name : string_view) : Result<UnitTy, EnvError> {
    comptime if(def.windows) {
        var r = SetEnvironmentVariableA(name.data(), null);
        if(r == 0) { return Result.Err(EnvError.OperationFailed("SetEnvironmentVariableA failed")); }
        return Result.Ok(UnitTy{});
    } else {
        var r = unsetenv(name.data());
        if(r != 0) { return Result.Err(EnvError.OperationFailed("unsetenv failed")); }
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

/// Get the current working directory (PWD on POSIX).
public func current_dir() : Option<string> {
    comptime if(!def.windows) {
        return get("PWD");
    }
    return Option.None<string>();
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
// OS-specific externs
// ---------------------------------------------------------------------------

@extern("getenv")
func getenv(name : *char) : *char

comptime if(!def.windows) {
    @extern("setenv")
    func setenv(name : *char, value : *char, overwrite : int) : int

    @extern("unsetenv")
    func unsetenv(name : *char) : int
}

comptime if(def.windows) {
    @extern("SetEnvironmentVariableA")
    func SetEnvironmentVariableA(name : *char, value : *char) : int
}

// ---------------------------------------------------------------------------
// Unit type
// ---------------------------------------------------------------------------

public struct UnitTy {}

} // end namespace environment
