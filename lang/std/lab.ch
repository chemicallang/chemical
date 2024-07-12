import "./string.ch"

enum ModuleType {
    File,
    CFile,
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

@no_init
struct Executable {
    var name : string
    var abs_path : string
}

struct BuildContext {

    var file_module : (&self, name : string, abs_path : string, dependencies : Module**, len : uint) => Module*;

    var files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : Module**, len : uint) => Module*;

    var c_file_module : (&self, name : string, path : string, dependencies : Module**, len : uint) => Module*

    var build_exe : (&self, name : string, dependencies : Module**, len : uint) => Executable*;

    var build_path : (&self) => string;

    var has_arg : (&self, name : string) => bool

    var get_arg : (&self, name : string) => string

    var remove_arg : (&self, name : string) => void

}