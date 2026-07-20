// ---------------------------------------------------------------------------
// Cross-platform WebView API
// ---------------------------------------------------------------------------

public namespace webview {

using std::Result;
using std::string;

// Create a new WebView with the given title, width, and height.
public func create(title : *char, width : int, height : int) : std::Result<WebView, WebViewError> {
    var wv = WebView.make()
    wv.title = string("")
    wv.title.append_char_ptr(title)
    wv.width = width
    wv.height = height

    var result = webview_create(&raw mut wv)
    if(result is Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create webview")))
    }

    return std.Result.Ok(wv)
}

// Convenience: create, load URL, and show
public func open_url(url : *char, title : *char, width : int, height : int) : std::Result<std::Unit, WebViewError> {
    var wv_result = create(title, width, height)
    if(wv_result is Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create webview")))
    }
    var Ok(wv) = wv_result else unreachable
    webview_load_url(&raw mut wv, url)
    webview_show(&raw mut wv)
    webview_run(&raw mut wv)
    webview_destroy(&raw mut wv)
    return std.Result.Ok(std::Unit{})
}

// Convenience: create, load HTML, and show
public func open_html(html : *char, title : *char, width : int, height : int) : std::Result<std::Unit, WebViewError> {
    var wv_result = create(title, width, height)
    if(wv_result is Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create webview")))
    }
    var Ok(wv) = wv_result else unreachable
    webview_load_html(&raw mut wv, html)
    webview_show(&raw mut wv)
    webview_run(&raw mut wv)
    webview_destroy(&raw mut wv)
    return std.Result.Ok(std::Unit{})
}

} // end namespace webview
