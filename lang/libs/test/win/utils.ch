func print_last_error(what : *char) {
    const err = GetLastError();
    var msg : LPVOID;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        null, err, 0, (&mut msg) as LPSTR, 0, null
    );
    fprintf(get_stderr(), "%s failed with error %lu: %s\n", what, err, msg as *char);
    LocalFree(msg);
}

func get_test_pipe_name(id : int) : std::string {
    var pipeName = std::string()
    pipeName.append_char_ptr("\\\\.\\pipe\\")
    pipeName.append_char_ptr("chem_test_pipe-");
    append_integer(pipeName, id);
    return pipeName;
}