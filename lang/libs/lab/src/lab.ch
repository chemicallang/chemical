public enum ModuleType {
    File,
    CFile,
    CPPFile,
    ObjFile,
    Directory
}

@compiler.interface
public interface Module {
    func getType(&self) : ModuleType
    func getScopeName(&self) : std::string_view
    func getName(&self) : std::string_view

    func getBitcodePath(&self) : std::string_view
    func setBitcodePath(&self, path : &std::string_view)

    func getObjectPath(&self) : std::string_view
    func setObjectPath(&self, path : &std::string_view)

    func getLlvmIrPath(&self) : std::string_view
    func setLlvmIrPath(&self, path : &std::string_view)

    func getAsmPath(&self) : std::string_view
    func setAsmPath(&self, path : &std::string_view)
}

public enum LabJobType {
    Executable,
    JITExecutable,
    Library,
    ToCTranslation,
    ToChemicalTranslation,
    ProcessingOnly,
    CBI
}

public enum LabJobStatus {
    Pending,
    Launched,
    Success,
    Failure
}

public enum OutputMode  : int {
    Debug,
    DebugQuick,
    DebugComplete,
    ReleaseFast,
    ReleaseSmall,
    ReleaseSafe
}

@compiler.interface
public interface LabJob {
    func getType(&self) : LabJobType
    func getName(&self) : std::string_view
    func getAbsPath(&self) : std::string_view
    func getBuildDir(&self) : std::string_view
    func getStatus(&self) : LabJobStatus
    func getTargetTriple(&self) : std::string_view
    func getMode(&self) : OutputMode
    func getTarget(&self) : &TargetData
}

@no_init
public struct LabJobCBI : LabJob {

}

public enum CBIFunctionType {
    InitializeLexer,
    ParseMacroValue,
    ParseMacroNode,
    ParseMacroTopLevelNode,
    ParseMacroMemberNode,
    SymResDeclareTopLevelNode,
    SymResLinkSignatureNode,
    SymResLinkSignatureValue,
    SymResNode,
    SymResValue,
    ReplacementNodeDeclare,
    ReplacementNode,
    ReplacementValue,
    SemanticTokensPut,
    FoldingRangesPut
}

public struct DependencySymbolInfo {
    var symbols : std::span<ImportSymbol>
    var alias : std::string_view
    var location : u64
};

public struct ModuleDependency {
    var module : *mut Module
    var info : *mut DependencySymbolInfo
};

@compiler.interface
public interface BuildContext {

    func getAnnotationController(&self) : *mut AnnotationController

    func new_module(&self, scope_name : &std::string_view, name : &std::string_view, dependencies : std::span<*Module>) : *mut Module

    func new_module_and_deps(&self, scope_name : &std::string_view, name : &std::string_view, dependencies : std::span<ModuleDependency>) : *mut Module

    func set_module_symbol_info(&self, module : *mut Module, index : uint, info : &DependencySymbolInfo);

    func get_cached(&self, job : *LabJob, scope_name : &std::string_view, name : &std::string_view) : *mut Module

    func set_cached(&self, job : *LabJob, module : *mut Module);

    func add_path(&self, module : *mut Module, path : &std::string_view);

    // adds the module to given job (as a dependency)
    func add_module(&self, job : *mut LabJob, module : *mut Module);

    // support's paths with .o, .c and .ch extensions
    func files_module (&self, scope_name : &std::string_view, name : &std::string_view, paths : **std::string_view, paths_len : uint, dependencies : std::span<*Module>) : *mut Module;

    // directory module
    func chemical_dir_module (&self, scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module

    // a single .c file
    func c_file_module (&self, scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module

    // a single .cpp file
    func cpp_file_module (&self, scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module

    // a single .o file
    func object_module (&self, scope_name : &std::string_view, name : &std::string_view, path : &std::string_view) : *mut Module

    // put the given before the existing job, so it is done before it
    func put_job_before(&self, newJob : *mut LabJob, existingJob : *mut LabJob);

    // would link this system library into the job of whoever consumes this module
    // the module parameter is optional
    func link_system_lib(&self, job : *mut LabJob, name : &std::string_view, module : *mut Module = null);

    // adds the given compiler interface to the module
    // returns true if it succeeds
    func add_compiler_interface(&self, module : *mut Module, interface : &std::string_view) : bool

    func resolve_condition(&self, job : *LabJob, condition : &std::string_view) : bool

    // allows to include c header in the module
    func include_header(&self, module : *mut Module, header : &std::string_view);

    // translate a module to chemical
    func translate_to_chemical (&self, module : *mut Module, output_path : &std::string_view) : *mut LabJob;

    // translate a chemical module to c file
    func translate_to_c (&self, name : &std::string_view, output_path : &std::string_view) : *mut LabJob

    // build executable using module dependencies
    func build_exe (&self, name : &std::string_view) : *mut LabJob;

    // compiles and runs the given executables instantly using jit
    func run_jit_exe (&self, name : &std::string_view) : *mut LabJob;

    // build a dynamic library using executable dependencies
    func build_dynamic_lib (&self, name : &std::string_view) : *mut LabJob;

    // build a cbi by given name, that can be used to integrate with compiler
    func build_cbi (&self, name : &std::string_view) : *mut LabJobCBI

    // sets the environment to testing mode
    // which means def.test for next executables would return this value
    // also ensures that resources for testing are ready
    // call this with true, if you want to generate a testing executable
    // however compiler wasn't invoked with --test parameter
    func set_environment_testing(&self, job : *mut LabJob, value : bool);

    // indexes a function from cbi, so it can be called when required
    func index_cbi_fn(&self, job : *mut LabJobCBI, key : &std::string_view, fn_name : &std::string_view, fn_type : CBIFunctionType) : bool

    // add a linkable object (.o file)
    func add_object (&self, job : *LabJob, path : &std::string_view) : void;

    // declare an alias for a path that can be used in imports like import '@alias/sub_path.ch'
    // returns true if declared
    func declare_alias (&self, job : *LabJob, alias : &std::string_view, path : &std::string_view) : bool;

    // get build
    func build_path (&self) : std::string_view;

    // check if argument given to chemical compiler
    // you can give argument using -arg-myarg, pass myarg to this function to check
    func has_arg (&self, name : &std::string_view) : bool

    // get the argument given to chemical compiler
    func get_arg (&self, name : &std::string_view) : std::string_view

    // remove the argument given to chemical compiler
    func remove_arg (&self, name : &std::string_view) : void

    // define a definition, that you can access using defined compiler function
    // returns true, if defined
    func define (&self, job : *LabJob, name : &std::string_view) : bool

    // un-define a definition
    func undefine (&self, job : *LabJob, name : &std::string_view) : bool;

    // invoke llvm dll tool with given cli args
    func invoke_dlltool (&self, string_arr : std::span<std::string_view>) : int;

    // invoke ranlib tool with given cli args
    func invoke_ranlib (&self, string_arr : std::span<std::string_view>) : int;

    // invoke lib tool with given cli args
    func invoke_lib (&self, string_arr : std::span<std::string_view>) : int;

    // invoke ar with given cli args
    func invoke_ar (&self, string_arr : std::span<std::string_view>) : int;

    // fetches a remote dependency for the job
    func fetch_job_dependency(&self, job : *mut LabJob, dep : &ImportRepo);

    // fetches a remote dependency for the module
    func fetch_mod_dependency(&self, job : *mut LabJob, mod : *mut Module, dep : &ImportRepo);

}

public struct ImportSymbol {
    var parts : std::span<std::string_view>;
    var alias : std::string_view;
};

public struct ImportRepo {
    var from : std::string_view
    var subdir : std::string_view
    var version : std::string_view
    var branch : std::string_view
    var commit : std::string_view
    var alias : std::string_view
    var symbols : std::span<ImportSymbol>
    var location : u64
}


@compiler.interface
public interface AppBuildContext : BuildContext {

    // launch an executable at the path
    func launch_executable (&self, path : &std::string_view, same_window : bool) : int;

    // something you'd want to be invoked when lab build has finished
    func on_finished (&self, lambda : (data : *void) => void, data : *void) : void;

}

public func (ctx : &BuildContext) add_compiler_interfaces(mod : *mut Module, interfaces : std::span<std::string_view>) {
    if(!interfaces.empty()) {
        var i : uint = 0;
        const s = interfaces.size()
        while(i < s) {
            const ci = interfaces.get(i)
            ctx.add_compiler_interface(mod, *ci)
            i++;
        }
    }
}

public func (ctx : &BuildContext) create_module(scope_name : &std::string_view, name : &std::string_view, dir_path : &std::string_view, dependencies : std::span<*Module>, interfaces : std::span<std::string_view>) : *mut Module {
    var mod = ctx.chemical_dir_module(scope_name, name, dir_path, dependencies)
    ctx.add_compiler_interfaces(mod, interfaces)
    return mod;
}

public func (ctx : &BuildContext) default_get(buildFlag : *mut bool, cached : *mut *mut Module, build : (ctx : *BuildContext) => *mut Module) : *mut Module {
    const c = *cached;
    if(c == null && *buildFlag == true) {
        const built = build(&ctx)
        *cached = built
        *buildFlag = false;
        return built;
    } else {
        return c;
    }
}

public func (ctx : &BuildContext) file_module(scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module {
    var path_ptr = &path;
    return ctx.files_module(scope_name, name, &path_ptr, 1, dependencies);
}

public func (ctx : &BuildContext) translate_file_to_chemical (c_path : &std::string_view, output_path : &std::string_view) : *mut LabJob {
    const deps : []*Module = []
    const mod = ctx.file_module(std::string_view(""), std::string_view("CFile"), c_path, deps);
    return ctx.translate_to_chemical(mod, output_path);
}

// allows to include headers in the module
public func (ctx : &BuildContext) include_headers(module : *mut Module, headers : std::span<std::string_view>) {
    var i = 0;
    const total = headers.size();
    while(i < total) {
        var ele = headers.get(i as size_t);
        ctx.include_header(module, *ele);
        i++;
    }
}

public func (ctx : &BuildContext) index_def_cbi_fn(job : *mut LabJobCBI, name : &std::string_view, type : CBIFunctionType) {
    ctx.index_cbi_fn(job, job.getName(), name, type)
}

// -----------------------------------------------------
// --------------- BUILD PATH FUNCTIONS ----------------
// -----------------------------------------------------

public func (ctx : &BuildContext) build_job_dir_path(job_name : &std::string_view) : std::string {
    const new_path = std::string(ctx.build_path());
    new_path.append('/');
    new_path.append_view(job_name);
    new_path.append_view(".dir");
    new_path.append('/')
    return new_path;
}

public func (ctx : &BuildContext) job_dir_path(job : *mut LabJob) : std::string {
    return ctx.build_job_dir_path(job.getName())
}

public func (ctx : &BuildContext) build_mod_file_path(job_name : &std::string_view, mod_scope : &std::string_view, mod_name : &std::string_view, file : &std::string_view) : std::string {
    var str = ctx.build_job_dir_path(job_name)
    str.append_view("modules/")
    if(!mod_scope.empty()) {
        str.append_view(mod_scope)
        str.append('.');
    }
    str.append_view(mod_name);
    str.append('/');
    str.append_view(file)
    return str;
}

public func (ctx : &BuildContext) build_llvm_ir_path(job_name : &std::string_view, mod_scope : &std::string_view, mod_name : &std::string_view) : std::string {
    return ctx.build_mod_file_path(job_name, mod_scope, mod_name, std::string_view("llvm_ir.ll"))
}

public func (ctx : &BuildContext) build_asm_path(job_name : &std::string_view, mod_scope : &std::string_view, mod_name : &std::string_view) : std::string {
    return ctx.build_mod_file_path(job_name, mod_scope, mod_name, std::string_view("mod_asm.s"))
}

public func (ctx : &BuildContext) build_bitcode_path(job_name : &std::string_view, mod_scope : &std::string_view, mod_name : &std::string_view) : std::string {
    return ctx.build_mod_file_path(job_name, mod_scope, mod_name, std::string_view("mod_bitcode.bc"))
}

public func (ctx : &BuildContext) llvm_ir_path(job : *mut LabJob, mod : *mut Module) : std::string {
    return ctx.build_llvm_ir_path(job.getName(), mod.getScopeName(), mod.getName())
}

public func (ctx : &BuildContext) asm_path(job : *mut LabJob, mod : *mut Module) : std::string {
    return ctx.build_asm_path(job.getName(), mod.getScopeName(), mod.getName())
}

public func (ctx : &BuildContext) bitcode_path(job : *mut LabJob, mod : *mut Module) : std::string {
    return ctx.build_bitcode_path(job.getName(), mod.getScopeName(), mod.getName())
}

public namespace lab {

    public func curr_dir_of(path : *char, len : size_t) : std::string {
        return fs::parent_path(std::string_view(path, len))
    }

    public comptime func curr_dir() : std::string {
        const call_loc = intrinsics::get_call_loc(9999) // this gets the first runtime call location to this function
        const loc_path = intrinsics::get_loc_file_path(call_loc)
        const loc_path_size = intrinsics::size(loc_path)
        return intrinsics::wrap(curr_dir_of(loc_path, loc_path_size)) as std::string
    }

    public func appended_str(str : std::string, path : *char) : std::string {
        str.append_char_ptr(path)
        return str;
    }

    public comptime func rel_path_to(path : *char) : std::string {
        return intrinsics::wrap(appended_str(curr_dir(), path)) as std::string
    }

}