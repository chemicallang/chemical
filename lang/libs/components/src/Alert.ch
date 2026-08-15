func alert_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: relative;
        display: flex;
        gap: 0.75rem;
        border-radius: var(--chx-radius-lg);
        border: 1px solid;
        padding: 0.875rem 1rem;
        font-size: 0.875rem;
        line-height: 1.5rem;
        background: hsl(var(--chx-card));
        color: hsl(var(--chx-foreground));
        &[data-variant="info"] {
            border-color: hsl(var(--chx-primary) / 0.3);
            background: hsl(var(--chx-primary) / 0.08);
        }
        &[data-variant="success"] {
            border-color: hsl(var(--chx-success) / 0.35);
            background: hsl(var(--chx-success) / 0.1);
        }
        &[data-variant="error"] {
            border-color: hsl(var(--chx-destructive) / 0.35);
            background: hsl(var(--chx-destructive) / 0.1);
        }
        &[data-variant="warning"] {
            border-color: hsl(var(--chx-warning) / 0.4);
            background: hsl(var(--chx-warning) / 0.12);
        }
        &[data-variant="accent"] {
            border-color: hsl(var(--chx-accent) / 0.35);
            background: hsl(var(--chx-accent) / 0.1);
        }
    }
}

func alert_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-weight: 600;
        font-size: 0.9375rem;
        line-height: 1.375rem;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func alert_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: hsl(var(--chx-muted-foreground));
        font-size: 0.875rem;
        line-height: 1.5rem;
        margin: 0.125rem 0 0 0;
    }
}

func alert_icon_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        margin-top: 0.125rem;
        flex-shrink: 0;
    }
}

func alert_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        flex: 1;
        min-width: 0;
    }
}

public #universal Alert(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var variant = props.variant || "default"
    var out = classes + " " + ${alert_styles(page)}
    return <div role="alert" data-variant={variant} class={out}>{props.children}</div>
}

public #universal AlertTitle(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h5 class={classes + " " + ${alert_title_styles(page)}}>{props.children}</h5>
}

public #universal AlertBody(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <p class={classes + " " + ${alert_body_styles(page)}}>{props.children}</p>
}

public #universal AlertIcon(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <span class={classes + " " + ${alert_icon_styles(page)}}>{props.children}</span>
}

public #universal AlertContent(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + ${alert_content_styles(page)}}>{props.children}</div>
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
