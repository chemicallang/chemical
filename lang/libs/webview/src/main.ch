// ---------------------------------------------------------------------------
// Cross-platform WebView API
// ---------------------------------------------------------------------------

public namespace webview {

using std::Result;
using std::string;

// Raw platform construction: allocate a `NativeWebView` from a C title. The
public func create_native(title : *char, width : int, height : int) : std::Result<NativeWebView, WebViewError> {
    var wv = NativeWebView.make()
    wv.title = string("")
    wv.title.append_char_ptr(title)
    wv.width = width
    wv.height = height

    var result = raw::webview_create(&raw mut wv)
    if(result is Result.Err) {
        var Err(err) = result else unsafe { unreachable }
        return std.Result.Err(WebViewError.InitFailed(err.message()))
    }

    return std.Result.Ok(wv)
}

// Raw convenience: create a native handle, load a URL, show, and run.
public func open_url_native(url : *char, title : *char, width : int, height : int) : std::Result<std::Unit, WebViewError> {
    var wv_result = create_native(title, width, height)
    if(wv_result is Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create webview")))
    }
    var Ok(wv) = wv_result else unsafe { unreachable }
    raw::webview_load_url(&raw mut wv, url)
    raw::webview_show(&raw mut wv)
    raw::webview_run(&raw mut wv)
    raw::webview_destroy(&raw mut wv)
    return std.Result.Ok(std::Unit{})
}

// Raw convenience: create a native handle, load HTML, show, and run.
public func open_html_native(html : *char, title : *char, width : int, height : int) : std::Result<std::Unit, WebViewError> {
    var wv_result = create_native(title, width, height)
    if(wv_result is Result.Err) {
        return std.Result.Err(WebViewError.InitFailed(string("failed to create webview")))
    }
    var Ok(wv) = wv_result else unsafe { unreachable }
    raw::webview_load_html(&raw mut wv, html)
    raw::webview_show(&raw mut wv)
    raw::webview_run(&raw mut wv)
    raw::webview_destroy(&raw mut wv)
    return std.Result.Ok(std::Unit{})
}

} // end namespace webview
