func launch_exe(path : &std::string_view) : int {

    var si : STARTUPINFOA
    var pi : PROCESS_INFORMATION;
    ZeroMemory(&mut si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&mut pi, sizeof(pi));

    var cmd = std::string()
    cmd.append_view(path)

    var ok = CreateProcessA(
        null,
        cmd.mutable_data(),
        null,
        null,
        false, // do not inherit handles
        0,
        null,
        null, // inherits cwd
        &mut si,
        &mut pi
    )

    if(!ok) {
         var e = GetLastError();
         fprintf(get_stderr(), "CreateProcess failed: %lu\n", e as ulong);
         return e as int;
    }

    // Wait for process to finish
    WaitForSingleObject(pi.hProcess, INFINITE as DWORD);

    var exitCode : DWORD;
    if (GetExitCodeProcess(pi.hProcess, &mut exitCode)) {
        // set the exit code in state
        state.exitCode = exitCode;
        if(exitCode != 0 && !state.fn.pass_on_crash) {
            state.has_failed = true;
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;

}