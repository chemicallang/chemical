// ErrorOverlay — a reusable global JavaScript error reporter.
//
// Drop `<ErrorOverlay />` anywhere in a `#universal` tree. On first client mount
// it installs global handlers for `window.onerror`, the `error` event and
// `unhandledrejection`, and shows a modal dialog with the full error message and
// stack trace whenever an uncaught error occurs. A "Copy" button puts the whole
// report on the clipboard so it can be pasted to a developer.
//
// It is intentionally dependency-free (no app imports) so it can live in the
// shared `components` library and be reused by any Chemical webview app.
//
// IMPORTANT: visibility is expressed as a JSX conditional used DIRECTLY as a
// child expression (`{open && errors.length > 0 ? <div>…</div> : null}`). This
// framework runs component bodies once and only re-renders reactive (derived)
// JSX nodes, so a control-flow `if(!open) return null` would never update when
// `open` flips. The styling uses inline `style={{...}}` objects (not `#css`
// helpers) because `#css` needs a server-only `page` pointer that is absent
// during client hydration.

public #universal ErrorOverlay(props) {
    var title = props.title || "Application Error"
    var max = props.max || 10
    state errors = []
    state open = false
    state selected = 0
    state copied = false

    useEffect(() => {
        var pushError = (info) => {
            var next = errors.concat([info])
            if(next.length > max) { next = next.slice(next.length - max) }
            errors = next
            selected = next.length - 1
            open = true
        }
        var onError = (e) => {
            var err = (e && e.error) ? e.error : null
            var msg = (e && e.message) ? e.message : "Unknown error"
            var stack = (err && err.stack) ? err.stack : msg
            if(!err && e && (e.filename || e.lineno)) {
                stack = stack + "\n  at " + (e.filename || "") + ":" + (e.lineno || 0) + ":" + (e.colno || 0)
            }
            pushError({
                time: new Date().toISOString(),
                message: msg,
                stack: stack,
                source: (e && e.filename) ? e.filename : "window.error",
                line: (e && e.lineno) ? e.lineno : 0,
                col: (e && e.colno) ? e.colno : 0
            })
        }
        var onRejection = (e) => {
            var reason = (e && e.reason) ? e.reason : "Unhandled promise rejection"
            var msg = (reason && reason.message) ? reason.message : ("" + reason)
            var stack = (reason && reason.stack) ? reason.stack : msg
            pushError({
                time: new Date().toISOString(),
                message: msg,
                stack: stack,
                source: "unhandledrejection",
                line: 0,
                col: 0
            })
        }
        if(window.addEventListener) {
            window.addEventListener("error", onError)
            window.addEventListener("unhandledrejection", onRejection)
        }
        window.__reportError = (msg, stack) => {
            pushError({
                time: new Date().toISOString(),
                message: (msg ? ("" + msg) : "Reported error"),
                stack: (stack ? ("" + stack) : ("" + msg)),
                source: "manual",
                line: 0,
                col: 0
            })
        }
        return () => {
            if(window.removeEventListener) {
                window.removeEventListener("error", onError)
                window.removeEventListener("unhandledrejection", onRejection)
            }
        }
    }, [])

    var dismiss = () => { open = false }
    var copyReport = () => {
        var cur = (errors.length > 0) ? errors[selected] : null
        var report = (cur ? (cur.message || "") : "") + "\n\n" + (cur ? (cur.stack || "") : "") + "\n\n[source: " + (cur ? (cur.source || "") : "") + " | time: " + (cur ? (cur.time || "") : "") + "]"
        try {
            if(navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(report)
            }
        } catch (e) { }
        copied = true
        setTimeout(() => { copied = false }, 2000)
    }

    return <div style={{ display: "contents" }}>
        {open && errors.length > 0 ? <div style={{
            position: "fixed",
            inset: "0",
            zIndex: "1000",
            display: "flex",
            alignItems: "flex-start",
            justifyContent: "center",
            padding: "2rem 1rem",
            background: "rgba(0,0,0,0.55)",
            overflow: "auto"
        }} onClick={dismiss}>
            <div style={{
                width: "100%",
                maxWidth: "46rem",
                borderRadius: "12px",
                border: "1px solid #e11d48",
                background: "#ffffff",
                color: "#111111",
                boxShadow: "0 10px 30px rgba(0,0,0,0.35)",
                display: "flex",
                flexDirection: "column",
                maxHeight: "80vh",
                overflow: "hidden",
                marginTop: "2rem"
            }} onClick={(e) => { e.stopPropagation() }}>
                <div style={{
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "space-between",
                    gap: "0.75rem",
                    padding: "0.875rem 1rem",
                    borderBottom: "1px solid #e5e7eb",
                    background: "rgba(225,29,72,0.08)"
                }}>
                    <h3 style={{ fontWeight: 600, fontSize: "0.95rem", margin: 0 }}>{title}</h3>
                    <button type="button" style={{ border: "none", background: "transparent", fontSize: "1.1rem", cursor: "pointer", color: "#111111" }} onClick={dismiss}>{"×"}</button>
                </div>
                <div style={{ padding: "1rem", overflow: "auto", display: "flex", flexDirection: "column", gap: "0.75rem" }}>
                    {errors.length > 1 ? <div style={{ display: "flex", flexWrap: "wrap", gap: "0.4rem" }}>
                        {errors.map((er, i) => (
                            <span style={{ fontSize: "0.72rem", padding: "0.15rem 0.5rem", borderRadius: "999px", border: "1px solid #e5e7eb", background: "#f3f4f6", cursor: "pointer" }} onClick={() => { selected = i }}>
                                {("#" + (i + 1) + " " + (er.source || "err"))}
                            </span>
                        ))}
                    </div> : null}
                    <p style={{ fontWeight: 600, fontSize: "0.9rem", color: "#e11d48", margin: 0, whiteSpace: "pre-wrap", wordBreak: "break-word" }}>{errors[selected].message}</p>
                    <pre style={{ margin: 0, padding: "0.75rem", borderRadius: "8px", background: "#f3f4f6", border: "1px solid #e5e7eb", fontFamily: "ui-monospace, SFMono-Regular, Menlo, Consolas, monospace", fontSize: "0.8rem", lineHeight: "1.35rem", whiteSpace: "pre-wrap", wordBreak: "break-word", maxHeight: "40vh", overflow: "auto" }}>{errors[selected].stack}</pre>
                </div>
                <div style={{ display: "flex", alignItems: "center", justifyContent: "flex-end", gap: "0.5rem", padding: "0.75rem 1rem", borderTop: "1px solid #e5e7eb" }}>
                    <span style={{ fontSize: "0.72rem", color: "#6b7280" }}>{"source: " + (errors[selected].source || "") + " @ " + (errors[selected].time || "")}</span>
                    <button type="button" style={{ border: "1px solid #e5e7eb", background: "#ffffff", borderRadius: "8px", padding: "0.35rem 0.7rem", cursor: "pointer" }} onClick={copyReport}>{copied ? "Copied!" : "Copy report"}</button>
                    <button type="button" style={{ border: "1px solid #e11d48", background: "#e11d48", color: "#ffffff", borderRadius: "8px", padding: "0.35rem 0.7rem", cursor: "pointer" }} onClick={dismiss}>{"Dismiss"}</button>
                </div>
            </div>
        </div> : null}
    </div>
}
