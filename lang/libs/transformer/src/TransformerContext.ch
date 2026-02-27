@compiler.interface
public struct TransformerContext {

    func getTargetJob(&self) : *LabJob

    func parseTarget(&self, keep_comments : bool) : bool

    func analyzeTarget(&self) : bool

    func getFlattenedModules(&self) : *mut VecRef<Module>

    func getFileTokens(&self, fileId : uint) : *mut VecRef<Token>

    func decodeLocation(&self, encoded : ubigint) : LocationData

}
