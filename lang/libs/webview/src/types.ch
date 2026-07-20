public namespace webview {

using std::string;

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
