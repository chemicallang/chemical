// Shadcn-style Toast system: ToastViewport + Toast (+ Toaster helper).
//
// Toast props:
//   title        optional bold title line
//   description  optional secondary line
//   variant      "default" (neutral) | "success" | "destructive" | "info" | "warning"
//   duration     auto-dismiss after N ms (default 4000); pass 0 to disable
//   onClose      fired when the toast is dismissed (manually or by timer)
//   action       optional JSX slot rendered on the right (buttons)
//   defaultVisible  uncontrolled initial visibility (default true)
//
// The Toast owns its visibility (uncontrolled) so a parent can simply render
// a toast inside a viewport when it should appear. Controlled mode: pass
// `visible` + `onClose` and render the toast conditionally from parent state.

func toast_viewport_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: fixed;
        bottom: 0;
        right: 0;
        z-index: 100;
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
        padding: 1rem;
        max-width: 100vw;
        width: 26rem;
        pointer-events: none;
        & > * {
            pointer-events: auto;
        }
    }
}

func toast_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        grid-template-columns: 1fr auto;
        gap: 0.75rem;
        padding: 0.9rem 1rem;
        border-radius: var(--radius-md);
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        box-shadow: var(--shadow-lg);
        animation: chx-toast-in 0.25s var(--ease);
        &[data-variant="success"] {
            border-color: hsl(var(--success) / 0.35);
        }
        &[data-variant="destructive"] {
            border-color: hsl(var(--destructive) / 0.4);
        }
        &[data-variant="info"] {
            border-color: hsl(var(--info) / 0.35);
        }
        &[data-variant="warning"] {
            border-color: hsl(var(--warning) / 0.4);
        }
    }
}

func toast_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.25rem;
        min-width: 0;
    }
}

func toast_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.875rem;
        font-weight: 600;
        line-height: 1.4;
    }
}

func toast_description_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.8125rem;
        color: hsl(var(--muted-foreground));
        line-height: 1.45;
        word-break: break-word;
    }
}

func toast_action_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 0.35rem 0.75rem;
        border-radius: 6px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--secondary));
        color: hsl(var(--secondary-foreground));
        font-size: 0.8125rem;
        font-weight: 500;
        cursor: pointer;
        white-space: nowrap;
        align-self: center;
        &:hover {
            background: hsl(var(--accent));
        }
    }
}

func toast_close_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 1.5rem;
        height: 1.5rem;
        border-radius: 4px;
        border: none;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-size: 1rem;
        line-height: 1;
        cursor: pointer;
        align-self: start;
        transition: background 0.15s ease, color 0.15s ease;
        &:hover {
            background: hsl(var(--accent));
            color: hsl(var(--foreground));
        }
    }
}

public #universal ToastViewport(props) {
    return <div {...props} class={${toast_viewport_styles(page)}}>{props.children}</div>
}

public #universal Toast(props) {
    state visible = props.defaultVisible != null ? props.defaultVisible : true
    var isVisible = props.visible != null ? props.visible : visible
    var variant = props.variant || "default"
    var duration = props.duration != null ? props.duration : 4000
    var dismiss = () => {
        if(props.visible == null) {
            visible = false
        }
        if(props.onClose) {
            props.onClose()
        }
    }
    useEffect(() => {
        if(!isVisible || duration <= 0) {
            return () => {}
        }
        const timer = setTimeout(() => {
            dismiss()
        }, duration)
        return () => clearTimeout(timer)
    }, [isVisible, duration])
    return <div {...props} data-variant={variant} role="status" class={${toast_styles(page)}} style={isVisible ? "" : "display:none;"}>
        <div class={${toast_body_styles(page)}}>
            {props.title ? <div class={${toast_title_styles(page)}}>{props.title}</div> : null}
            {props.description ? <div class={${toast_description_styles(page)}}>{props.description}</div> : null}
            {props.children}
        </div>
        {props.action ? <div class={${toast_action_styles(page)}} onClick={props.actionClick}>{props.action}</div> : null}
        <button type="button" aria-label="Close" class={${toast_close_styles(page)}} onClick={dismiss}>×</button>
    </div>
}

// Toaster: a ready-made viewport container. Render <Toaster> once per page
// and drop <Toast> children into it from your app.
public #universal Toaster(props) {
    return <ToastViewport {...props}>{props.children}</ToastViewport>
}
