public enum ModuleType {
    File,
    CFile,
    CPPFile,
    ObjFile,
    Directory
}

@no_init
public struct Module {
    var type : ModuleType
    var scope_name : std::string
    var name : std::string
    // a path can be given, to output the translated C file (if any)
    var out_c_path : std::string
    // the bitcode file path for this module
    var bitcode_path : std::string;
    // the object file path for this module
    var object_path : std::string;
    // if not empty, module's llvm ir is written to at this path
    var llvm_ir_path : std::string;
    // if not empty, module's assembly is written to at this path
    var asm_path : std::string;
}

public enum LabJobType {
    Executable,
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

@no_init
public struct LabJob {
    var type : LabJobType
    var name : std::string
    var abs_path : std::string
    var build_dir : std::string
    var status : LabJobStatus
}

@no_init
public struct LabJobCBI : LabJob {

}

public enum CBIType {
    MacroLexer,
    MacroParser,
    IDESupport
}

public enum CBIFunctionType {
    InitializeLexer,
    ParseMacroValue,
    ParseMacroNode,
    SemanticTokensPut,
    FoldingRangesPut
}

public struct PathResolutionResult {
    var path : std::string
    var error : std::string
}

@compiler.interface
public struct BuildContext {

    /**
     * find the given module (if its build.lab / chemical.mod has been built)
     */
    func get_module(&self, scope_name : &std::string_view, name : &std::string_view) : *mut Module;

    func new_module(&self, scope_name : &std::string_view, name : &std::string_view, dependencies : std::span<*Module>) : *mut Module

    func add_path(&self, module : *mut Module, path : &std::string_view);

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

    // adds the given compiler interface to the module
    // returns true if it succeeds
    func add_compiler_interface(&self, module : *mut Module, interface : &std::string_view) : bool

    // resolve the path for a native library given the scope name and mod name
    func resolve_native_lib_path(&self, scope_name : &std::string_view, mod_name : &std::string_view) : PathResolutionResult

    // resolves a path, this allows to get exact path to the library or file
    // you can resolve for example where the std library is using base_path empty and path "@std/"
    func resolve_import_path(&self, base_path : &std::string_view, path : &std::string_view) : PathResolutionResult

    // allows to include c header in the module
    func include_header(&self, module : *mut Module, header : &std::string_view);

    // translate a module to chemical
    func translate_to_chemical (&self, module : *mut Module, output_path : &std::string_view) : *mut LabJob;

    // translate a chemical module to c file
    func translate_to_c (&self, name : &std::string_view, dependencies : std::span<*Module>, output_dir : &std::string_view) : *mut LabJob

    // build executable using module dependencies
    func build_exe (&self, name : &std::string_view, dependencies : std::span<*Module>) : *mut LabJob;

    // build a dynamic library using executable dependencies
    func build_dynamic_lib (&self, name : &std::string_view, dependencies : std::span<*Module>) : *mut LabJob;

    // build a cbi by given name, that can be used to integrate with compiler
    func build_cbi (&self, name : &std::string_view, dependencies : std::span<*Module>) : *mut LabJobCBI

    // indexes a function from cbi, so it can be called when required
    func index_cbi_fn(&self, job : *mut LabJobCBI, key : &std::string_view, fn_name : &std::string_view, fn_type : CBIFunctionType) : bool

    // add a linkable object (.o file)
    func add_object (&self, job : *LabJob, path : &std::string_view) : void;

    // declare an alias for a path that can be used in imports like import '@alias/sub_path.ch'
    // returns true if declared
    func declare_alias (&self, job : *LabJob, alias : &std::string_view, path : &std::string_view) : bool;

    // get build
    func build_path (&self) : std::string;

    // check if argument given to chemical compiler
    // you can give argument using -arg-myarg, pass myarg to this function to check
    func has_arg (&self, name : &std::string_view) : bool

    // get the argument given to chemical compiler
    func get_arg (&self, name : &std::string_view) : std::string

    // remove the argument given to chemical compiler
    func remove_arg (&self, name : &std::string_view) : void

    // define a definition, that you can access using defined compiler function
    // returns true, if defined
    func define (&self, job : *LabJob, name : &std::string_view) : bool

    // un-define a definition
    func undefine (&self, job : *LabJob, name : &std::string_view) : bool;

    // link object files (.o files) into a single binary
    func link_objects (&self, string_arr : std::span<std::string_view>, output_path : &std::string_view) : int;

    // invoke llvm dll tool with given cli args
    func invoke_dlltool (&self, string_arr : std::span<std::string_view>) : int;

    // invoke ranlib tool with given cli args
    func invoke_ranlib (&self, string_arr : std::span<std::string_view>) : int;

    // invoke lib tool with given cli args
    func invoke_lib (&self, string_arr : std::span<std::string_view>) : int;

    // invoke ar with given cli args
    func invoke_ar (&self, string_arr : std::span<std::string_view>) : int;

    // DEPRECATED: because we should not support importing files (.lab / .mod) that are imported conditionally
    // using another build.lab, this is because to import another file, we must completely build the .lab file
    // which means another module is built while there's a module for root .lab file in existence
    // therefore this method has been removed, this creates some restrictions like not being able to copy
    // a directory in the project and then importing it, this is intentional, we may remove these restrictions
    // by providing a prebuild support invoked from command line
    // the benefits are
    // 1 - all the build.lab / chemical.mod files form a single module (a single tree of files)
    // 2 - its performant because we parse all files in a multi-threaded fashion
    // 3 - syntax for importing is nicer
    // 4 - chemical.mod files are translated automatically, great interoperability
    // 5 - the most important benefit is that the build methods in lab files are called once
    //   - because user calls get, which checks a local variable before calling build
    // 6 - the entirety of lab files' modules remain inside memory, which means user
    //   - can give us a function pointer (from any lab/module) and we'll be able to call it later
    //   - however if this method is used, we dispose the build.lab after module pointer is received
    // /**
    //  * create a module from a directory that contains a chemical.mod or build.lab file, the scope name and mod name is given to check if module by that name already
    //  * has been parsed so we return it fast, the path is the absolute path to directory, the returned module may be null, in that case error is set to error_msg
    //  * it can be that an unknown error happened and we didn't set the error, however module is still null, important to check returned module for nullability
    //  */
    // func module_from_directory(&self, path : &std::string_view, scope_name : &std::string_view, mod_name : &std::string_view, error_msg : &mut std::string) : *mut Module

}

@compiler.interface
public struct AppBuildContext : BuildContext {

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

// DEPRECATED: this method relies on module_from_directory which is deprecated
// please read that method's description
// public func (ctx : &BuildContext) native_lib_module(mod : &std::string_view) : *mut Module {
//     var path_res = ctx.resolve_native_lib_path(std::string_view(""), mod)
//     if(path_res.error.empty()) {
//         printf("resolved module %s path to be: %s\n", mod.data(), path_res.path.data());
//     } else {
//         return null;
//     }
//     var err = std::string()
//     const module = ctx.module_from_directory(std::string_view(path_res.path), std::string_view(""), mod, err)
//     if(module == null && !err.empty()) {
//         printf("error '%s' during creation of native module '%s'\n", err.data(), mod.data());
//     }
//     return module;
// }

public func (ctx : &BuildContext) file_module(scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module {
    const path_ptr = &path;
    return ctx.files_module(scope_name, name, &path_ptr, 1, dependencies);
}

public func (ctx : &BuildContext) translate_file_to_chemical (c_path : &std::string_view, output_path : &std::string_view) : *mut LabJob {
    const mod = ctx.file_module(std::string_view(""), std::string_view("CFile"), c_path, [  ]);
    return ctx.translate_to_chemical(mod, output_path);
}

public func (ctx : &BuildContext) translate_mod_to_c(module : *Module, output_dir : &std::string_view) : *mut LabJob {
    return ctx.translate_to_c(std::string_view("ToCJob"), [ module ], output_dir);
}

public func (ctx : &BuildContext) translate_file_to_c(chem_path : &std::string, output_path : &std::string_view) : *mut LabJob {
    var mod = ctx.file_module(std::string_view(""), std::string_view("TempChem"), chem_path.to_view(), []);
    return ctx.translate_mod_to_c(mod, output_path);
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
    var fn_name = job.name.copy()
    fn_name.append_with_len(name.data(), name.size())
    ctx.index_cbi_fn(job, job.name.to_view(), fn_name.to_view(), type)
}

// add the given cbi to a cbi job
public func (ctx : &BuildContext) add_cbi_type(job : *mut LabJobCBI, type : CBIType) : bool {
    switch(type) {
        CBIType.MacroLexer => {
            ctx.index_def_cbi_fn(job, std::string_view("_initializeLexer"), CBIFunctionType.InitializeLexer);
        }
        CBIType.MacroParser => {
            ctx.index_def_cbi_fn(job, std::string_view("_parseMacroValue"), CBIFunctionType.ParseMacroValue);
            ctx.index_def_cbi_fn(job, std::string_view("_parseMacroNode"), CBIFunctionType.ParseMacroNode);
        }
        CBIType.IDESupport => {
            ctx.index_def_cbi_fn(job, std::string_view("_semanticTokensPut"), CBIFunctionType.SemanticTokensPut);
            ctx.index_def_cbi_fn(job, std::string_view("_foldingRangesPut"), CBIFunctionType.FoldingRangesPut);
        }
    }
    return true;
}

// -----------------------------------------------------
// --------------- BUILD PATH FUNCTIONS ----------------
// -----------------------------------------------------

public func (ctx : &BuildContext) build_job_dir_path(job_name : &std::string_view) : std::string {
    const new_path = ctx.build_path();
    new_path.append('/');
    new_path.append_with_len(job_name.data(), job_name.size());
    new_path.append_char_ptr(".dir");
    new_path.append('/')
    return new_path;
}

public func (ctx : &BuildContext) job_dir_path(job : *mut LabJob) : std::string {
    return ctx.build_job_dir_path(job.name.to_view())
}

public func (ctx : &BuildContext) build_mod_file_path(job_name : &std::string_view, mod_scope : &std::string_view, mod_name : &std::string_view, file : &std::string_view) : std::string {
    var str = ctx.build_job_dir_path(job_name)
    if(!mod_scope.empty()) {
        str.append_with_len(mod_scope.data(), mod_scope.size())
        str.append('.');
    }
    str.append_with_len(mod_name.data(), mod_name.size());
    str.append('/');
    str.append_with_len(file.data(), file.size())
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
    return ctx.build_llvm_ir_path(job.name.to_view(), mod.scope_name.to_view(), mod.name.to_view())
}

public func (ctx : &BuildContext) asm_path(job : *mut LabJob, mod : *mut Module) : std::string {
    return ctx.build_asm_path(job.name.to_view(), mod.scope_name.to_view(), mod.name.to_view())
}

public func (ctx : &BuildContext) bitcode_path(job : *mut LabJob, mod : *mut Module) : std::string {
    return ctx.build_bitcode_path(job.name.to_view(), mod.scope_name.to_view(), mod.name.to_view())
}

public namespace lab {

    public func curr_dir_of(path : *char, len : size_t) : std::string {
        return fs::parent_path(std::string_view(path, len))
    }

    @comptime
    public func curr_dir() : std::string {
        const call_loc = intrinsics::get_call_loc(9999) // this gets the first runtime call location to this function
        const loc_path = intrinsics::get_loc_file_path(call_loc)
        const loc_path_size = intrinsics::size(loc_path)
        return intrinsics::wrap(curr_dir_of(loc_path, loc_path_size)) as std::string
    }

    public func appended_str(str : std::string, path : *char) : std::string {
        str.append_char_ptr(path)
        return str;
    }

    @comptime
    public func rel_path_to(path : *char) : std::string {
        return intrinsics::wrap(appended_str(curr_dir(), path)) as std::string
    }

}