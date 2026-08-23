public namespace webview {

using std::string;
using std::string_view;

// Callback invoked with the result of webview_evaluate_js_result.
// `result` is a JSON-encoded string (as returned by the engine — e.g. a JS
// number arrives as "42", a JS string as "\"hello\"", an object as its JSON
// text) and is valid only during the call. `data` is the user data passed to
// webview_evaluate_js_result. On evaluation failure `result` is null.
public type JsResultCallback = (data : *mut void, result : *char) => void

// Handler invoked for every JS->native bridge call. `method` is the bridge
// method name and `args` is the JSON array of arguments exactly as passed to
// window.__webview__.call(method, ...args) (e.g. `["hello"]`). The handler
// returns a JSON-encoded string; the engine surfaces it to the calling
// JavaScript as the resolved value of the returned Promise. Runs on the UI
// thread (inside the message loop), asynchronously with the page's JavaScript.
public type JsBindHandler = std.function<(method : string_view, args : string_view) => string>

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
