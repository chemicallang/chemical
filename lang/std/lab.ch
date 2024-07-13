import "./string.ch"

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
    // if not empty, module is translated to c at this path
    var translate_c_path : string;
}

enum LabJobType {
    Executable,
    Library
}

@no_init
struct LabJob {
    var type : LabJobType
    var name : string
    var abs_path : string
    var build_dir : string
}

struct BuildContext {

    // support's paths with .o, .c and .ch extensions
    var files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : Module**, len : uint) => Module*;

    // when paths only contain chemical files
    var chemical_files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : Module**, len : uint) => Module*;

    // a single .c file
    var c_file_module : (&self, name : string, path : string, dependencies : Module**, len : uint) => Module*

    // a single .o file
    var object_module : (&self, name : string, path : string) => Module*

    var build_exe : (&self, name : string, dependencies : Module**, len : uint) => LabJob*;

    var build_dynamic_lib : (&self, name : string, dependencies : Module**, len : uint) => LabJob*;

    var add_object : (&self, job : LabJob*, path : string) => void;

    var build_path : (&self) => string;

    var has_arg : (&self, name : string) => bool

    var get_arg : (&self, name : string) => string

    var remove_arg : (&self, name : string) => void

    func chemical_file_module(&self, name : string, path : string, dependencies : Module**, len : uint) : Module* {
        return chemical_files_module(name, &path, 1, dependencies, len);
    }

    func file_module(&self, name : string, path : string, dependencies : Module**, len : uint) : Module* {
        return files_module(name, &path, 1, dependencies, len);
    }

}