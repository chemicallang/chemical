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

    var add_with_type : (&self, type : ModuleType, name : string*, abs_path : string*, dependencies : Module**, len : uint) => Module*;

    var build_exe : (&self, name : string*, dependencies : Module**, len : uint) => Executable*;

}