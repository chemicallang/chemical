import "@std/string.ch"
import "@std/array_ref.ch"
import "@std/fs.ch"

using namespace std;

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
    var name : string
    // a path can be given, to output the translated C file (if any)
    var out_c_path : string
    // the bitcode file path for this module
    var bitcode_path : string;
    // the object file path for this module
    var object_path : string;
    // if not empty, module's llvm ir is written to at this path
    var llvm_ir_path : string;
    // if not empty, module's assembly is written to at this path
    var asm_path : string;
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
    var name : string
    var abs_path : string
    var build_dir : string
    var status : LabJobStatus
}

public enum CBIImportKind {
    Lexer
}

struct PathResolutionResult {
    var path : string
    var error : string
}

@compiler.interface
public struct BuildContext {

    // support's paths with .o, .c and .ch extensions
    func files_module (&self, name : &string, paths : **string, paths_len : uint, dependencies : ArrayRef<*Module>) : *mut Module;

    // when paths only contain chemical files
    func chemical_files_module (&self, name : &string, paths : **string, paths_len : uint, dependencies : ArrayRef<*Module>) : *mut Module;

    // directory module
    func chemical_dir_module (&self, name : &string, path : &string, dependencies : ArrayRef<*Module>) : *mut Module

    // a single .c file
    func c_file_module (&self, name : &string, path : &string, dependencies : ArrayRef<*Module>) : *mut Module

    // a single .cpp file
    func cpp_file_module (&self, name : &string, path : &string, dependencies : ArrayRef<*Module>) : *mut Module

    // a single .o file
    func object_module (&self, name : &string, path : &string) : *mut Module

    // resolves a path, this allows to get exact path to the library or file
    // you can resolve for example where the std library is using base_path empty and path "@std/"
    func resolve_import_path(&self, base_path : &string, path : &string) : PathResolutionResult

    // allows to include c header in the module
    func include_header(&self, module : *mut Module, header : &string);

    // allows to include a chemical file in the module
    func include_file(&self, module : *mut Module, abs_path : &string);

    // translate a module to chemical
    func translate_to_chemical (&self, module : *mut Module, output_path : &string) : *mut LabJob;

    // translate a chemical module to c file
    func translate_to_c (&self, name : &string, dependencies : ArrayRef<*Module>, output_dir : &string) : *mut LabJob

    // build executable using module dependencies
    func build_exe (&self, name : *string, dependencies : ArrayRef<*Module>) : *mut LabJob;

    // build a dynamic library using executable dependencies
    func build_dynamic_lib (&self, name : *string, dependencies : ArrayRef<*Module>) : *mut LabJob;

    // build a cbi by given name, that can be used to integrate with compiler
    func build_cbi (&self, name : *string, entry : *Module, dependencies : ArrayRef<*Module>) : *mut LabJob

    // add a linkable object (.o file)
    func add_object (&self, job : *LabJob, path : &string) : void;

    // declare an alias for a path that can be used in imports like import '@alias/sub_path.ch'
    // returns true if declared
    func declare_alias (&self, job : *LabJob, alias : &string, path : &string) : bool;

    // get build
    func build_path (&self) : string;

    // check if argument given to chemical compiler
    // you can give argument using -arg-myarg, pass myarg to this function to check
    func has_arg (&self, name : &string) : bool

    // get the argument given to chemical compiler
    func get_arg (&self, name : &string) : string

    // remove the argument given to chemical compiler
    func remove_arg (&self, name : &string) : void

    // define a definition, that you can access using defined compiler function
    // returns true, if defined
    func define (&self, job : *LabJob, name : &string) : bool

    // un-define a definition
    func undefine (&self, job : *LabJob, name : &string) : bool;

    // launch an executable at the path
    func launch_executable (&self, path : &string, same_window : bool) : int;

    // something you'd want to be invoked when lab build has finished
    func on_finished (&self, lambda : (data : *void) => void, data : *void) : void;

    // link object files (.o files) into a single binary
    func link_objects (&self, string_arr : ArrayRef<string>, output_path : &string) : int;

    // invoke llvm dll tool with given cli args
    func invoke_dlltool (&self, string_arr : ArrayRef<string>) : int;

    // invoke ranlib tool with given cli args
    func invoke_ranlib (&self, string_arr : ArrayRef<string>) : int;

    // invoke lib tool with given cli args
    func invoke_lib (&self, string_arr : ArrayRef<string>) : int;

    // invoke ar with given cli args
    func invoke_ar (&self, string_arr : ArrayRef<string>) : int;

}

func (ctx : &BuildContext) chemical_file_module(name : &string, path : &string, dependencies : ArrayRef<*Module>) : *mut Module {
    const path_ptr = &path;
    return ctx.chemical_files_module(name, &path_ptr, 1, dependencies);
}

func (ctx : &BuildContext) file_module(name : &string, path : &string, dependencies : ArrayRef<*Module>) : *mut Module {
    const path_ptr = &path;
    return ctx.files_module(name, &path_ptr, 1, dependencies);
}

func (ctx : &BuildContext) translate_file_to_chemical (c_path : &string, output_path : &string) : *mut LabJob {
    const mod = ctx.file_module(string("CFile"), c_path, {  });
    return ctx.translate_to_chemical(mod, output_path);
}

func (ctx : &BuildContext) translate_mod_to_c(module : *Module, output_dir : &string) : *mut LabJob {
    return ctx.translate_to_c(string("ToCJob"), { module }, output_dir);
}

func (ctx : &BuildContext) translate_file_to_c(chem_path : &string, output_path : &string) : *mut LabJob {
    var mod = ctx.file_module(string("TempChem"), chem_path, {});
    return ctx.translate_mod_to_c(mod, output_path);
}

// allows to include headers in the module
func (ctx : &BuildContext) include_headers(module : *mut Module, headers : ArrayRef<string>) {
    var i = 0;
    const total = headers.size();
    while(i < total) {
        var ele = headers.get(i as size_t);
        ctx.include_header(module, *ele);
        i++;
    }
}

// allows to include headers in the module
func (ctx : &BuildContext) include_files(module : *mut Module, files : ArrayRef<string>) {
    var i = 0;
    const total = files.size();
    while(i < total) {
        var ele = files.get(i as size_t);
        ctx.include_file(module, *ele);
        i++;
    }
}

namespace lab {

    private func curr_dir_of(path : *char, len : size_t) : string {
        return fs::parent_path(std::string_view(path, len))
    }

    @comptime
    func curr_dir() : string {
        const call_loc = compiler::get_call_loc(9999) // this gets the first runtime call location to this function
        const loc_path = compiler::get_loc_file_path(call_loc)
        const loc_path_size = compiler::size(loc_path)
        return compiler::wrap(curr_dir_of(loc_path, loc_path_size))
    }

    private func appended_str(str : string, path : *char) : string {
        str.append_char_ptr(path)
        return str;
    }

    @comptime
    func rel_path_to(path : *char) : string {
        return compiler::wrap(appended_str(curr_dir(), path))
    }

}