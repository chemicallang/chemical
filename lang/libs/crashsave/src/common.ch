// Struct to hold one stack frame
public struct StackFrame {
    var pc : uintptr_t;
    var filename : std::string
    var lineno : int
    var function : std::string;
};