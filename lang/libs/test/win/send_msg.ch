func (env : &mut TestEnvImpl) send_message(msg : *char, len : size_t) {
    unsafe var written : DWORD;
    if (!WriteFile(env.pipeHandle, msg, len as DWORD, &raw mut written, null)) {
        print_last_error("WriteFile");
    }
}
