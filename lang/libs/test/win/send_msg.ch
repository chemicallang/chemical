func (env : &mut TestEnvImpl) send_message(msg : *char, len : size_t) {
    unsafe {
    if(env.pipeHandle == null || env.pipeHandle == INVALID_HANDLE_VALUE) { return }
    if(msg == null && len > 0u) { return }
    if(len > 0xFFFFFFFF as size_t) { return }
    var written : DWORD;
    if (!WriteFile(env.pipeHandle, msg, len as DWORD, &raw mut written, null)) {
        print_last_error("WriteFile");
    }
    }
}
