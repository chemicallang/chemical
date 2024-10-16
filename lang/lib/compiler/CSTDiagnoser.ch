import "./CSTToken.ch"
import "@std/string.ch"

public enum DiagSeverity {
    // Reports an error.
    Error,
    // Reports a warning.
    Warning,
    // Reports an information.
    Information,
    // Reports a hint.
    Hint
}

@compiler.interface
public struct CSTDiagnoser {

    func put_diagnostic(&self, msg : &string, start : *CSTToken, end : *CSTToken, severity : DiagSeverity);

}

public func (diagnoser : &CSTDiagnoser) diagnostic(msg : &string, inside : *CSTToken, severity : DiagSeverity) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), severity);
}

public func (diagnoser : &CSTDiagnoser) hint(msg : &string, inside : *CSTToken) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Hint);
}

public func (diagnoser : &CSTDiagnoser) info(msg : &string, inside : *CSTToken) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Information);
}

public func (diagnoser : &CSTDiagnoser) warn(msg : &string, inside : *CSTToken) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Warning);
}

public func (diagnoser : &CSTDiagnoser) error(msg : &string, inside : *CSTToken) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Error);
}