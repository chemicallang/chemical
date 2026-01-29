public namespace md {

using std::Result;

public enum MdError {
    FileReadFailed,
}

public func to_html(text : std::string_view) : std::string {
    var arena = Arena()
    var toks = lex(text)
    var root = parse(&mut toks, &mut arena)
    return md::render_to_html(root)
}

public func file_to_html(path : *char) : Result<std::string, MdError> {
    var r = fs::read_entire_file(path)
    if(r is Result.Err) {
        return Result.Err<std::string, MdError>(MdError.FileReadFailed)
    }
    var Ok(bytes) = r else unreachable
    const view = std::string_view(bytes.data() as *char, bytes.size())
    const html = md::to_html(view)
    return Result.Ok<std::string, MdError>(html)
}

}
