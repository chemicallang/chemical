@compiler.interface
public interface ASTDiagnoser {

    func error(&self, msg : &std::string_view, loc : ubigint);

}