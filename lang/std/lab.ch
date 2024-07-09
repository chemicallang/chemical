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

struct BuildContext {

    var add_with_type : (&self, type : ModuleType, name : string*, abs_path : string*, dependencies : Module**, len : uint) => Module*;

}