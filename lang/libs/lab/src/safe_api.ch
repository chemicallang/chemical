// lab::safe — additive safe facade over the raw build API.
//
// The `BuildContext`/`LabJob`/`Module` interfaces and their raw-pointer
// extension helpers stay available unchanged for `build.lab` scripts. This
// file adds a small, non-generic surface:
//
//   * typed `ModuleId`/`JobId` newtypes (the raw handle stays private)
//   * `Result`-returning free functions for the common build operations
//   * `lab::run(command : &str) : Result<string, LabError>`
//
// Deliberately no borrowing `ModuleId<'a>`/`JobId<'a>` handles or
// `SafeBuildContext` wrapper here: the compiler's generic-instantiation pass
// does not currently link method bodies of a lifetime-only generic struct, so
// a typed-handle newtype is the largest shape that is provably sound and
// compiles today.

public namespace lab {

// Canonical typed handles (the raw types stay at module scope for the CBI ABI).
public type ModuleId = safe::ModuleId;
public type JobId = safe::JobId;
public type LabError = safe::LabError;

public namespace safe {

using std::Result;
using std::string;
using std::string_view;
using std::vector;
using namespace raw;

/** Failure modes of the safe build/command facade. */
public variant LabError {
    /// The build context returned a null module/job handle.
    NullHandle()
    /// The text was not representable (empty name/path or interior NUL).
    InvalidText()
    /// `popen` could not start the command.
    SpawnFailed()
    /// The command ran but exited with a non-zero status.
    CommandFailed(status : int)
}

// -------------------------------------------------------------------------
// Typed handles (private raw pointer; checked accessors only)
// -------------------------------------------------------------------------

/** A checked handle to a build module. */
@direct_init
public struct ModuleId {
    internal var ptr : *mut Module

    public func valid(&self) : bool {
        return self.ptr != null
    }

    public func name(&self) : string_view {
        if(self.ptr == null) {
            return string_view("")
        }
        return unsafe(self.ptr.getName())
    }

    public func scope_name(&self) : string_view {
        if(self.ptr == null) {
            return string_view("")
        }
        return unsafe(self.ptr.getScopeName())
    }

    public func kind(&self) : ModuleType {
        if(self.ptr == null) {
            return ModuleType.File
        }
        return unsafe(self.ptr.getType())
    }
}

/** A checked handle to a build job. */
@direct_init
public struct JobId {
    internal var ptr : *mut LabJob

    public func valid(&self) : bool {
        return self.ptr != null
    }

    public func name(&self) : string_view {
        if(self.ptr == null) {
            return string_view("")
        }
        return unsafe(self.ptr.getName())
    }

    public func build_dir(&self) : string_view {
        if(self.ptr == null) {
            return string_view("")
        }
        return unsafe(self.ptr.getBuildDir())
    }

    public func kind(&self) : LabJobType {
        if(self.ptr == null) {
            return LabJobType.Executable
        }
        return unsafe(self.ptr.getType())
    }

    public func status(&self) : LabJobStatus {
        if(self.ptr == null) {
            return LabJobStatus.Failure
        }
        return unsafe(self.ptr.getStatus())
    }
}

// -------------------------------------------------------------------------
// Module construction / wiring
// -------------------------------------------------------------------------

func text_view(text : &str) : string_view {
    return text.to_view()
}

func module_dependencies(dependencies : std::span<ModuleId>) : vector<ModuleDependency> {
    var deps = vector<ModuleDependency>()
    var i : size_t = 0
    while(i < dependencies.size()) {
        const id = unsafe(*dependencies.get(i))
        deps.push(ModuleDependency {
            module : id.ptr,
            info : null
        })
        i += 1
    }
    return deps
}

/**
 * Create a library module (`PackageKind.Library`).
 *
 * `dependencies` is a slice of typed module handles; the raw dependency
 * records are built internally.
 */
public func new_module(ctx : &BuildContext, type : ModuleType, scope_name : &str, name : &str, dependencies : std::span<ModuleId>) : Result<ModuleId, LabError> {
    if(scope_name.empty() || name.empty()) {
        return Result.Err<ModuleId, LabError>(LabError.InvalidText())
    }
    const scope_view = text_view(scope_name)
    const name_view = text_view(name)
    var deps = module_dependencies(dependencies)
    const dep_view = unsafe(std::span<ModuleDependency>(deps.data(), deps.size()))
    const module = ctx.new_module(type, &scope_view, &name_view, dep_view)
    if(module == null) {
        return Result.Err<ModuleId, LabError>(LabError.NullHandle())
    }
    return Result.Ok<ModuleId, LabError>(ModuleId { ptr : module })
}

/** Create an application module (`PackageKind.Application`). */
public func new_app_module(ctx : &BuildContext, type : ModuleType, scope_name : &str, name : &str, dependencies : std::span<ModuleId>) : Result<ModuleId, LabError> {
    if(scope_name.empty() || name.empty()) {
        return Result.Err<ModuleId, LabError>(LabError.InvalidText())
    }
    const scope_view = text_view(scope_name)
    const name_view = text_view(name)
    var deps = module_dependencies(dependencies)
    const dep_view = unsafe(std::span<ModuleDependency>(deps.data(), deps.size()))
    const module = ctx.new_app_module(type, &scope_view, &name_view, dep_view)
    if(module == null) {
        return Result.Err<ModuleId, LabError>(LabError.NullHandle())
    }
    return Result.Ok<ModuleId, LabError>(ModuleId { ptr : module })
}

/** Add a source/object path to a module. */
public func add_path(ctx : &BuildContext, module : ModuleId, path : &str) : Result<bool, LabError> {
    if(module.ptr == null || path.empty()) {
        return Result.Err<bool, LabError>(LabError.InvalidText())
    }
    const view = text_view(path)
    ctx.add_path(unsafe(module.ptr), &view)
    return Result.Ok<bool, LabError>(true)
}

/** Attach `module` to `job`. */
public func add_module(ctx : &BuildContext, job : JobId, module : ModuleId) {
    if(job.ptr != null && module.ptr != null) {
        ctx.add_module(job.ptr, module.ptr)
    }
}

/** Attach `module` as a dependency of `job`. */
public func add_dependency(ctx : &BuildContext, job : JobId, module : ModuleId) {
    if(job.ptr != null && module.ptr != null) {
        ctx.add_dependency(job.ptr, module.ptr, null)
    }
}

// -------------------------------------------------------------------------
// Jobs
// -------------------------------------------------------------------------

func job_from_raw(raw : *mut LabJob) : Result<JobId, LabError> {
    if(raw == null) {
        return Result.Err<JobId, LabError>(LabError.NullHandle())
    }
    return Result.Ok<JobId, LabError>(JobId { ptr : raw })
}

public func build_exe(ctx : &BuildContext, name : &str) : Result<JobId, LabError> {
    if(name.empty()) {
        return Result.Err<JobId, LabError>(LabError.InvalidText())
    }
    const view = text_view(name)
    return job_from_raw(ctx.build_exe(&view))
}

public func run_jit_exe(ctx : &BuildContext, name : &str) : Result<JobId, LabError> {
    if(name.empty()) {
        return Result.Err<JobId, LabError>(LabError.InvalidText())
    }
    const view = text_view(name)
    return job_from_raw(ctx.run_jit_exe(&view))
}

public func build_dynamic_lib(ctx : &BuildContext, name : &str) : Result<JobId, LabError> {
    if(name.empty()) {
        return Result.Err<JobId, LabError>(LabError.InvalidText())
    }
    const view = text_view(name)
    return job_from_raw(ctx.build_dynamic_lib(&view))
}

public func build_interpretation(ctx : &BuildContext, name : &str) : Result<JobId, LabError> {
    if(name.empty()) {
        return Result.Err<JobId, LabError>(LabError.InvalidText())
    }
    const view = text_view(name)
    return job_from_raw(ctx.build_interpretation(&view))
}

public func build_cbi(ctx : &BuildContext, name : &str) : Result<JobId, LabError> {
    if(name.empty()) {
        return Result.Err<JobId, LabError>(LabError.InvalidText())
    }
    const view = text_view(name)
    return job_from_raw(ctx.build_cbi(&view) as *mut LabJob)
}

// -------------------------------------------------------------------------
// Build arguments and paths
// -------------------------------------------------------------------------

public func build_path(ctx : &BuildContext) : string_view {
    return ctx.build_path()
}

public func has_arg(ctx : &BuildContext, name : &str) : bool {
    const view = text_view(name)
    return ctx.has_arg(&view)
}

public func get_arg(ctx : &BuildContext, name : &str) : string_view {
    const view = text_view(name)
    return ctx.get_arg(&view)
}

// -------------------------------------------------------------------------
// Command execution
// -------------------------------------------------------------------------

/**
 * Run `command` through the C library shell and capture its standard output.
 *
 * The command is a NUL-terminated C string internally; an interior NUL byte is
 * reported as `LabError.InvalidText`. A non-zero exit status is a typed error,
 * not a panic.
 */
public func run(command : &str) : Result<string, LabError> {
    var made = std::cstring_make(command)
    if(made is Result.Err) {
        return Result.Err<string, LabError>(LabError.InvalidText())
    }
    var Ok(cmd) = made else unsafe { unreachable }

    var pipe = unsafe(popen(unsafe(cmd.as_ptr()), "r"))
    if(pipe == null) {
        return Result.Err<string, LabError>(LabError.SpawnFailed())
    }

    var output = string()
    var buffer : [4096]char
    while(true) {
        var read = unsafe(fread(&raw mut buffer[0] as *mut void, 1, 4096, pipe))
        if(read == 0) {
            break
        }
        output.append_with_len(&raw buffer[0], read)
    }

    var status = unsafe(pclose(pipe))
    if(status != 0) {
        return Result.Err<string, LabError>(LabError.CommandFailed(status))
    }
    return Result.Ok<string, LabError>(output)
}

} // namespace safe

// -------------------------------------------------------------------------
// Top-level aliases (the documented `lab::run` / `lab::new_module` spellings)
// -------------------------------------------------------------------------

/** Safe command runner; see `lab::safe::run`. */
public func run(command : &str) : std::Result<std::string, safe::LabError> {
    return safe::run(command)
}

/** Saturated module, see `lab::safe::new_module`. */
public func safe_new_module(ctx : &BuildContext, type : ModuleType, scope_name : &str, name : &str, dependencies : std::span<safe::ModuleId>) : std::Result<safe::ModuleId, safe::LabError> {
    return safe::new_module(ctx, type, scope_name, name, dependencies)
}

} // namespace lab
