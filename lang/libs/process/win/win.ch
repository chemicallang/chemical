// Windows implementation of process management.
//
// execute / spawn / wait are not implemented on Windows yet (the public API
// in process.ch reports "not implemented" for them). kill() works via
// TerminateProcess. This file exists so the module compiles on Windows and
// follows the win/posix source layout used by the other libraries.
//
// Note: the TerminateProcess declaration must match the one in
// lang/libs/test/win/launch.ch exactly — both end up in the same C
// translation unit for the test executables and a mismatch would be a
// redefinition error.

public namespace process {

@dllimport
@extern
@stdcall
public func TerminateProcess(hProcess : HANDLE, uExitCode : UINT) : BOOL;

} // end namespace process
