import "./string.ch"

enum ModuleType {
    RootFile,
    Directory
}

@no_init
struct Module {
    var type : ModuleType
    var name : string
    var path : string
}

struct BuildContext {

    var add_with_type : (type : LabModuleType, name : string, path : string, dependencies : Module**, len : uint) => Module*;

}