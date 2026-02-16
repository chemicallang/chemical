public namespace md {

using std::Result;

public enum MdError {
    FileReadFailed,
}

public func to_html(text : std::string_view, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) : std::string {
    var arena = Arena()
    var toks = lex(text)
    var root = parse(&toks, &mut arena)
    return render_to_html(root, highlighter, link_rewriter)
}

public func file_to_html(path : *char, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) : Result<std::string, MdError> {
    var r = fs::read_entire_file(path)
    if(r is Result.Err) {
        var Err(value) = r else unreachable;
        printf("couldn't read the file %s\n", value.message().data());
        return Result.Err<std::string, MdError>(MdError.FileReadFailed)
    }
    var Ok(bytes) = r else unreachable
    const view = std::string_view(bytes.data() as *char, bytes.size())
    const html = md::to_html(view, highlighter, link_rewriter)
    return Result.Ok<std::string, MdError>(html)
}

}
