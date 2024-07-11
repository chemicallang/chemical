import "./string.ch"

enum ModuleType {
    File,
    Directory
}

@no_init
struct Module {
    var type : ModuleType
    var name : string
    var path : string
}

@no_init
struct Executable {
    var name : string
    var abs_path : string
}

struct BuildContext {

    var file_module : (&self, name : string, abs_path : string, dependencies : Module**, len : uint) => Module*;

    var files_module : (&self, name : string, paths : string**, paths_len : uint, dependencies : Module**, len : uint) => Module*;

    var build_exe : (&self, name : string, dependencies : Module**, len : uint) => Executable*;

    var has_arg : (&self, name : string) => bool

    var get_arg : (&self, name : string) => string

    var remove_arg : (&self, name : string) => void

}