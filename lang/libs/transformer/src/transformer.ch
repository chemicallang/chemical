@compiler.interface
public interface ASTFileMetaData {
    func getFileId(&self) : uint
    func getAbsPath(&self) : std::string_view
    func getFileScope(&self) : *mut FileScope
}

@compiler.interface
public interface TransformerModule : Module {
    func getFiles(&self) : std::span<ASTFileMetaData>
    func getFileCount(&self) : uint
    func getFile(&self, index : uint) : *mut ASTFileMetaData
    func getDependencyCount(&self) : uint
    func getDependency(&self, index : uint) : *mut TransformerModule
}

@compiler.interface
public interface TransformerFileScope : FileScope {
    func getBody(&self) : *mut Scope
}
