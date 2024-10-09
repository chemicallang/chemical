import "./CSTToken.ch"
import "@std/string.ch"

enum DiagSeverity {
    // Reports an error.
    Error,
    // Reports a warning.
    Warning,
    // Reports an information.
    Information,
    // Reports a hint.
    Hint
}

@compiler:interface
struct CSTDiagnoser {

    func put_diagnostic(&self, msg : &string, start : CSTToken*, end : CSTToken*, severity : DiagSeverity);

}

func (diagnoser : CSTDiagnoser*) diagnostic(msg : &string, inside : CSTToken*, severity : DiagSeverity) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), severity);
}

func (diagnoser : CSTDiagnoser*) hint(msg : &string, inside : CSTToken*) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Hint);
}

func (diagnoser : CSTDiagnoser*) info(msg : &string, inside : CSTToken*) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Information);
}

func (diagnoser : CSTDiagnoser*) warn(msg : &string, inside : CSTToken*) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Warning);
}

func (diagnoser : CSTDiagnoser*) error(msg : &string, inside : CSTToken*) {
    diagnoser.put_diagnostic(msg, inside.start_token(), inside.end_token(), DiagSeverity.Error);
}