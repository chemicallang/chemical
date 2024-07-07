enum ModuleType {
    Directory
}

@no_init
struct Module {
    var name : string
    var type : ModuleType
}

struct BuildContext {

    var dir_module : (name : const string, path : string, dependencies : Module*[]) => Module*;

}