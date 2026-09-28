// Shadcn-style alert. `variant` picks the tone (default/info/success/error/
// warning/accent); `title`/`description` props render the composed layout in
// one call; `dismissible` shows a close button that fires `onDismiss`.
// className merges with the generated style class; other props pass through.
public #universal Alert(props) {
    var alert = style {
        position: relative;
        display: flex;
        gap: 0.75rem;
        border-radius: var(--radius-lg);
        border: 1px solid;
        padding: 0.875rem 1rem;
        font-size: 0.875rem;
        line-height: 1.5rem;
        background: hsl(var(--card));
        color: hsl(var(--foreground));
        &[data-variant="info"] {
            border-color: hsl(var(--primary) / 0.3);
            background: hsl(var(--primary) / 0.08);
        }
        &[data-variant="success"] {
            border-color: hsl(var(--success) / 0.35);
            background: hsl(var(--success) / 0.1);
        }
        &[data-variant="error"] {
            border-color: hsl(var(--destructive) / 0.35);
            background: hsl(var(--destructive) / 0.1);
        }
        &[data-variant="warning"] {
            border-color: hsl(var(--warning) / 0.4);
            background: hsl(var(--warning) / 0.12);
        }
        &[data-variant="accent"] {
            border-color: hsl(var(--accent) / 0.35);
            background: hsl(var(--accent) / 0.1);
        }
    }
    var alert_content = style {
        flex: 1;
        min-width: 0;
    }
    var alert_dismiss = style {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 1.5rem;
        height: 1.5rem;
        margin: -0.25rem -0.25rem -0.25rem 0;
        border-radius: 9999px;
        border: 0;
        background: transparent;
        color: inherit;
        opacity: 0.6;
        cursor: pointer;
        flex-shrink: 0;
        font-size: 0.9375rem;
        line-height: 1;
        transition: opacity 0.15s ease, background 0.15s ease;
        &:hover {
            opacity: 1;
            background: hsl(var(--foreground) / 0.08);
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring) / 1);
            outline-offset: 2px;
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var variant = props.variant || "default"
    var dismissible = props.dismissible || false
    var out = classes + " " + alert
    var close = () => {
        if(props.onDismiss) { props.onDismiss() }
    }
    if(props.title !== undefined || props.description !== undefined) {
        return <div role="alert" data-variant={variant} class={out}>
            <div class={alert_content}>
                {props.title !== undefined ? <AlertTitle>{props.title}</AlertTitle> : null}
                {props.description !== undefined ? <AlertDescription>{props.description}</AlertDescription> : null}
            </div>
            {props.children}
            {dismissible ? <button type="button" class={alert_dismiss} onClick={close} aria-label="Dismiss alert">{"×"}</button> : null}
        </div>
    }
    return <div role="alert" data-variant={variant} class={out}>
        <div class={alert_content}>{props.children}</div>
        {dismissible ? <button type="button" class={alert_dismiss} onClick={close} aria-label="Dismiss alert">{"×"}</button> : null}
    </div>
}

public #universal AlertTitle(props) {
    var alert_title = style {
        font-weight: 600;
        font-size: 0.9375rem;
        line-height: 1.375rem;
        margin: 0;
        color: hsl(var(--foreground));
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h5 class={classes + " " + alert_title}>{props.children}</h5>
}

public #universal AlertBody(props) {
    var alert_body = style {
        color: hsl(var(--muted-foreground));
        font-size: 0.875rem;
        line-height: 1.5rem;
        margin: 0.125rem 0 0 0;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <p class={classes + " " + alert_body}>{props.children}</p>
}

// Shadcn name for the body text — AlertBody kept as a legacy alias.
public #universal AlertDescription(props) {
    return <AlertBody {...props}>{props.children}</AlertBody>
}

public #universal AlertIcon(props) {
    var alert_icon = style {
        display: inline-flex;
        align-items: center;
        margin-top: 0.125rem;
        flex-shrink: 0;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <span class={classes + " " + alert_icon}>{props.children}</span>
}

public #universal AlertContent(props) {
    var alert_content = style {
        flex: 1;
        min-width: 0;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + alert_content}>{props.children}</div>
}

// Legacy aliases
public #universal AlertAccent(props) {
    return <Alert {...props} variant="accent">{props.children}</Alert>
}
public #universal AlertSuccess(props) {
    return <Alert {...props} variant="success">{props.children}</Alert>
}
public #universal AlertError(props) {
    return <Alert {...props} variant="error">{props.children}</Alert>
}
public #universal AlertWarning(props) {
    return <Alert {...props} variant="warning">{props.children}</Alert>
}
public #universal AlertInfo(props) {
    return <Alert {...props} variant="info">{props.children}</Alert>
}

// Destructive variant (shadcn naming convention)
public #universal AlertDestructive(props) {
    return <Alert {...props} variant="error">{props.children}</Alert>
}

// Shadcn AlertAction: positioned absolutely in the top-right corner of the alert.
public #universal AlertAction(props) {
    var alert_action = style {
        position: absolute;
        top: 0.75rem;
        right: 0.75rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + alert_action}>{props.children}</div>
}
