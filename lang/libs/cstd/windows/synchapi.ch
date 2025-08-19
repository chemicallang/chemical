@extern
@dllimport
@stdcall
public func WaitForSingleObject(
    hHandle : HANDLE,
    dwMilliseconds : DWORD
) : DWORD