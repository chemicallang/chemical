public namespace webview {

using std::string;

// Callback invoked with the result of webview_evaluate_js_result.
// `result` is a JSON-encoded string (as returned by the engine — e.g. a JS
// number arrives as "42", a JS string as "\"hello\"", an object as its JSON
// text) and is valid only during the call. `data` is the user data passed to
// webview_evaluate_js_result. On evaluation failure `result` is null.
public type JsResultCallback = (data : *mut void, result : *char) => void

public variant WebViewError {
    PlatformNotSupported()
    InitFailed(msg : string)
    NavigationFailed(msg : string)
    JsEvaluationFailed(msg : string)
    BindFailed(name : string)

    func message(&self) : string {
        switch(self) {
            PlatformNotSupported() => return string("WebViewError: platform not supported")
            InitFailed(msg) => {
                var s = string("WebViewError: init failed: ")
                s.append_string(&msg)
                return s
            }
            NavigationFailed(msg) => {
                var s = string("WebViewError: navigation failed: ")
                s.append_string(&msg)
                return s
            }
            JsEvaluationFailed(msg) => {
                var s = string("WebViewError: JS evaluation failed: ")
                s.append_string(&msg)
                return s
            }
            BindFailed(name) => {
                var s = string("WebViewError: bind failed for: ")
                s.append_string(&name)
                return s
            }
        }
    }
}

} // end namespace webview
