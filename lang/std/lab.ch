import "./string.ch"
import "./array_ref.ch"

enum ModuleType {
    File,
    CFile,
    ObjFile,
    Directory
}

@no_init
struct Module {
    var type : ModuleType
    var name : string
    // the bitcode file path for this module
    var bitcode_path : string;
    // the object file path for this module
    var object_path : string;
    // if not empty, module's llvm ir is written to at this path
    var llvm_ir_path : string;
    // if not empty, module's assembly is written to at this path
    var asm_path : string;
}

enum LabJobType {
    Executable,
    Library,
    ToCTranslation,
    ToChemicalTranslation
}

enum LabJobStatus {
    Pending,
    Launched,
    Success,
    Failure
}

@no_init
struct LabJob {
    var type : LabJobType
    var name : string
    var abs_path : string
    var build_dir : string
    var status : LabJobStatus
}

struct BuildContext {

    // support's paths with .o, .c and .ch extensions
    var files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : ArrayRef<Module*>) => Module*;

    // when paths only contain chemical files
    var chemical_files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : ArrayRef<Module*>) => Module*;

    // a single .c file
    var c_file_module : (&self, name : string, path : string, dependencies : ArrayRef<Module*>) => Module*

    // a single .o file
    var object_module : (&self, name : string, path : string) => Module*

    // translate a c file to chemical
    var translate_to_chemical : (&self, c_path : string, output_path : string) => LabJob*;

    // translate a chemical module to c file
    var translate_mod_to_c : (&self, module : Module*, output_path : string) => LabJob*

    // build executable using module dependencies
    var build_exe : (&self, name : string, dependencies : ArrayRef<Module*>) => LabJob*;

    // build a dynamic library using executable dependencies
    var build_dynamic_lib : (&self, name : string, dependencies : ArrayRef<Module*>) => LabJob*;

    // add a linkable object (.o file)
    var add_object : (&self, job : LabJob*, path : string) => void;

    // get build
    var build_path : (&self) => string;

    // check if argument given to chemical compiler
    // you can give argument using -arg-myarg, pass myarg to this function to check
    var has_arg : (&self, name : string) => bool

    // get the argument given to chemical compiler
    var get_arg : (&self, name : string) => string

    // remove the argument given to chemical compiler
    var remove_arg : (&self, name : string) => void

    // launch an executable at the path
    var launch_executable : (&self, path : string, same_window : bool) => int;

    // something you'd want to be invoked when lab build has finished
    var on_finished : (&self, lambda : (data : void*) => void, data : void*) => void;

    // link object files (.o files) into a single binary
    var link_objects : (&self, string_arr : ArrayRef<string>, output_path : string) => int;

    // invoke llvm dll tool with given cli args
    var invoke_dlltool : (&self, string_arr : ArrayRef<string>) => int;

    // invoke ranlib tool with given cli args
    var invoke_ranlib : (&self, string_arr : ArrayRef<string>) => int;

    // invoke lib tool with given cli args
    var invoke_lib : (&self, string_arr : ArrayRef<string>) => int;

    // invoke ar with given cli args
    var invoke_ar : (&self, string_arr : ArrayRef<string>) => int;

    func chemical_file_module(&self, name : string, path : string, dependencies : ArrayRef<Module*>) : Module* {
        return chemical_files_module(name, &path, 1, dependencies);
    }

    func file_module(&self, name : string, path : string, dependencies : ArrayRef<Module*>) : Module* {
        return files_module(name, &path, 1, dependencies);
    }

    func translate_to_c(&self, chem_path : string, output_path : string) : LabJob* {
        var mod = file_module("TempChem", chem_path, {});
        return translate_mod_to_c(mod, output_path);
    }

}