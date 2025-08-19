
@extern
@dllimport
@stdcall
public func GetExitCodeProcess(
    hProcess : HANDLE,
    lpExitCode : LPDWORD
) : bool