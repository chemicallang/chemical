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
// Optional props:
//   title        header text (default "Application Error")
//   max          max number of captured errors kept (default 10)
//   variant      accent color token (default "destructive")
//
// App code that swallows errors (e.g. a bridge `.catch`) can still surface them
// by calling `window.__reportError(message, stack?)` — the overlay listens for
// it and opens the dialog just like a real uncaught error.

func error_overlay_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: fixed;
        inset: 0;
        z-index: 1000;
        display: flex;
        align-items: flex-start;
        justify-content: center;
        padding: 2rem 1rem;
        background: hsl(var(--background) / 0.6);
        backdrop-filter: blur(2px);
        overflow: auto;
    }
}

func error_overlay_dialog_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        max-width: 46rem;
        border-radius: var(--radius-lg);
        border: 1px solid hsl(var(--destructive) / 0.4);
        background: hsl(var(--card));
        color: hsl(var(--foreground));
        box-shadow: var(--shadow-lg);
        display: flex;
        flex-direction: column;
        max-height: 80vh;
        overflow: hidden;
    }
}

func error_overlay_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.875rem 1rem;
        border-bottom: 1px solid hsl(var(--border));
        background: hsl(var(--destructive) / 0.08);
    }
}

func error_overlay_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-weight: 600;
        font-size: 0.95rem;
        margin: 0;
    }
}

func error_overlay_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1rem;
        overflow: auto;
        display: flex;
        flex-direction: column;
        gap: 0.75rem;
    }
}

func error_overlay_msg_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-weight: 600;
        font-size: 0.9rem;
        color: hsl(var(--destructive));
        margin: 0;
        white-space: pre-wrap;
        word-break: break-word;
    }
}

func error_overlay_pre_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        padding: 0.75rem;
        border-radius: var(--radius-md);
        background: hsl(var(--muted) / 0.5);
        border: 1px solid hsl(var(--border));
        font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
        font-size: 0.8rem;
        line-height: 1.35rem;
        white-space: pre-wrap;
        word-break: break-word;
        max-height: 40vh;
        overflow: auto;
    }
}

func error_overlay_list_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        flex-wrap: wrap;
        gap: 0.4rem;
    }
}

func error_overlay_chip_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.72rem;
        padding: 0.15rem 0.5rem;
        border-radius: 999px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--muted) / 0.4);
        cursor: pointer;
    }
}

func error_overlay_footer_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.5rem;
        padding: 0.75rem 1rem;
        border-top: 1px solid hsl(var(--border));
    }
}

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

    if(!open) { return null }

    var total = errors.length
    var idx = selected
    if(idx < 0) { idx = 0 }
    if(idx >= total) { idx = total - 1 }
    var current = errors[idx]
    var reportText = (current.message || "") + "\n\n" + (current.stack || "") + "\n\n[source: " + (current.source || "") + " | time: " + (current.time || "") + "]"

    var copyReport = () => {
        try {
            if(navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(reportText)
            }
        } catch (e) { }
        copied = true
        setTimeout(() => { copied = false }, 2000)
    }
    var dismiss = () => { open = false }

    return <div class={error_overlay_styles(page)} onClick={dismiss}>
        <div class={error_overlay_dialog_styles(page)} onClick={(e) => { e.stopPropagation() }}>
            <div class={error_overlay_header_styles(page)}>
                <h3 class={error_overlay_title_styles(page)}>{title}</h3>
                <button type="button" class="cdm-btn" onClick={dismiss}>{"×"}</button>
            </div>
            <div class={error_overlay_body_styles(page)}>
                {total > 1 ? <div class={error_overlay_list_styles(page)}>
                    {errors.map((er, i) => (
                        <span class={error_overlay_chip_styles(page)} onClick={() => { selected = i }}>
                            {("#" + (i + 1) + " " + (er.source || "err"))}
                        </span>
                    ))}
                </div> : null}
                <p class={error_overlay_msg_styles(page)}>{current.message}</p>
                <pre class={error_overlay_pre_styles(page)}>{current.stack}</pre>
            </div>
            <div class={error_overlay_footer_styles(page)}>
                <span style={{ fontSize: "0.72rem", color: "hsl(var(--muted-foreground))" }}>{"source: " + (current.source || "") + " @ " + (current.time || "")}</span>
                <button type="button" class="cdm-btn" onClick={copyReport}>{copied ? "Copied!" : "Copy report"}</button>
                <button type="button" class="cdm-btn" data-variant="primary" onClick={dismiss}>{"Dismiss"}</button>
            </div>
        </div>
    </div>
}
