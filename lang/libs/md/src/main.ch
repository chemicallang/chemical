public namespace md {

using std::Result;

public enum MdError {
    FileReadFailed,
}

public func to_html(text : std::string_view) : std::string {
    var arena = Arena()
    var toks = lex(text)
    var root = parse(&toks, &mut arena)
    return render_to_html(root)
}

public func file_to_html(path : *char) : Result<std::string, MdError> {
    var r = fs::read_entire_file(path)
    if(r is Result.Err) {
        var Err(value) = r else unreachable;
        printf("couldn't read the file %s\n", value.message().data());
        return Result.Err<std::string, MdError>(MdError.FileReadFailed)
    }
    var Ok(bytes) = r else unreachable
    const view = std::string_view(bytes.data() as *char, bytes.size())
    const html = md::to_html(view)
    return Result.Ok<std::string, MdError>(html)
}

}
