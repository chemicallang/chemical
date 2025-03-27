import "@std/string.ch"
import "@std/string_view.ch"
import "@std/span.ch"
import "@std/fs.ch"
import "@std/std.ch"

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
public struct LabJobCBI {

}

public enum CBIType {
    MacroLexer,
    MacroParser
}

public struct PathResolutionResult {
    var path : std::string
    var error : std::string
}

@compiler.interface
public struct BuildContext {

    /**
     * create a module from a directory that contains a chemical.mod or build.lab file, the scope name and mod name is given to check if module by that name already
     * has been parsed so we return it fast, the path is the absolute path to directory, the returned module may be null, in that case error is set to error_msg
     * it can be that an unknown error happened and we didn't set the error, however module is still null, important to check returned module for nullability
     */
    func module_from_directory(&self, path : &std::string_view, scope_name : &std::string_view, mod_name : &std::string_view, error_msg : &mut std::string) : *mut Module

    // support's paths with .o, .c and .ch extensions
    func files_module (&self, scope_name : &std::string_view, name : &std::string_view, paths : **std::string_view, paths_len : uint, dependencies : std::span<*Module>) : *mut Module;

    // when paths only contain chemical files
    func chemical_files_module (&self, scope_name : &std::string_view, name : &std::string_view, paths : **std::string_view, paths_len : uint, dependencies : std::span<*Module>) : *mut Module;

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
    func build_exe (&self, name : *std::string_view, dependencies : std::span<*Module>) : *mut LabJob;

    // build a dynamic library using executable dependencies
    func build_dynamic_lib (&self, name : *std::string_view, dependencies : std::span<*Module>) : *mut LabJob;

    // build a cbi by given name, that can be used to integrate with compiler
    func build_cbi (&self, name : *std::string_view, dependencies : std::span<*Module>) : *mut LabJobCBI

    // add the given cbi to a cbi job
    func add_cbi_type(&self, job : *mut LabJobCBI, type : CBIType) : bool

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

    // launch an executable at the path
    func launch_executable (&self, path : &std::string_view, same_window : bool) : int;

    // something you'd want to be invoked when lab build has finished
    func on_finished (&self, lambda : (data : *void) => void, data : *void) : void;

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

}

public func (ctx : &BuildContext) chemical_file_module(scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module {
    const path_ptr = &path;
    return ctx.chemical_files_module(scope_name, name, &path_ptr, 1, dependencies);
}

public func (ctx : &BuildContext) file_module(scope_name : &std::string_view, name : &std::string_view, path : &std::string_view, dependencies : std::span<*Module>) : *mut Module {
    const path_ptr = &path;
    return ctx.files_module(scope_name, name, &path_ptr, 1, dependencies);
}

public func (ctx : &BuildContext) translate_file_to_chemical (c_path : &std::string_view, output_path : &std::string_view) : *mut LabJob {
    const mod = ctx.file_module(std::string_view(""), std::string_view("CFile"), c_path, {  });
    return ctx.translate_to_chemical(mod, output_path);
}

public func (ctx : &BuildContext) translate_mod_to_c(module : *Module, output_dir : &std::string_view) : *mut LabJob {
    return ctx.translate_to_c(std::string_view("ToCJob"), { module }, output_dir);
}

public func (ctx : &BuildContext) translate_file_to_c(chem_path : &std::string, output_path : &std::string_view) : *mut LabJob {
    var mod = ctx.file_module(std::string_view(""), std::string_view("TempChem"), chem_path, {});
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

public namespace lab {

    private func curr_dir_of(path : *char, len : size_t) : std::string {
        return fs::parent_path(std::string_view(path, len))
    }

    @comptime
    func curr_dir() : std::string {
        const call_loc = compiler::get_call_loc(9999) // this gets the first runtime call location to this function
        const loc_path = compiler::get_loc_file_path(call_loc)
        const loc_path_size = compiler::size(loc_path)
        return compiler::wrap(curr_dir_of(loc_path, loc_path_size))
    }

    private func appended_str(str : std::string, path : *char) : std::string {
        str.append_char_ptr(path)
        return str;
    }

    @comptime
    func rel_path_to(path : *char) : std::string {
        return compiler::wrap(appended_str(curr_dir(), path))
    }

}