func badge_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.375rem;
        border-radius: 9999px;
        border: 1px solid transparent;
        padding: 0.125rem 0.625rem;
        font-size: 0.75rem;
        line-height: 1.25rem;
        font-weight: 600;
        white-space: nowrap;
        transition: colors 0.15s ease;
        background: hsl(var(--chx-muted));
        color: hsl(var(--chx-muted-foreground));
        &[data-variant="secondary"] {
            background: hsl(var(--chx-secondary));
            color: hsl(var(--chx-secondary-foreground));
        }
        &[data-variant="accent"] {
            background: hsl(var(--chx-accent));
            color: hsl(var(--chx-accent-foreground));
        }
        &[data-variant="success"] {
            background: hsl(var(--chx-success));
            color: hsl(var(--chx-success-foreground));
        }
        &[data-variant="error"] {
            background: hsl(var(--chx-destructive));
            color: hsl(var(--chx-destructive-foreground));
        }
        &[data-variant="warning"] {
            background: hsl(var(--chx-warning));
            color: hsl(var(--chx-warning-foreground));
        }
        &[data-variant="outline"] {
            border-color: hsl(var(--chx-border));
            color: hsl(var(--chx-foreground));
            background: transparent;
        }
        &[data-variant="outline-success"] {
            border-color: hsl(var(--chx-success));
            color: hsl(var(--chx-success));
            background: transparent;
        }
        &[data-variant="outline-error"] {
            border-color: hsl(var(--chx-destructive));
            color: hsl(var(--chx-destructive));
            background: transparent;
        }
        &[data-size="sm"] {
            font-size: 0.625rem;
            padding: 0.0625rem 0.5rem;
        }
        &[data-size="lg"] {
            font-size: 0.875rem;
            padding: 0.25rem 0.875rem;
        }
    }
}

public #universal Badge(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var variant = props.variant || "default"
    var size = props.size || "md"
    var out = classes + " " + ${badge_styles(page)}
    return <span data-variant={variant} data-size={size} class={out}>{props.children}</span>
}

// Legacy aliases
public #universal BadgeAccent(props) {
    return <Badge {...props} variant="accent">{props.children}</Badge>
}
public #universal BadgeSuccess(props) {
    return <Badge {...props} variant="success">{props.children}</Badge>
}
public #universal BadgeError(props) {
    return <Badge {...props} variant="error">{props.children}</Badge>
}
public #universal Chip(props) {
    return <Badge {...props} variant="outline">{props.children}</Badge>
}
public #universal ChipAccent(props) {
    return <Badge {...props} variant="accent">{props.children}</Badge>
}
public #universal ChipSuccess(props) {
    return <Badge {...props} variant="success">{props.children}</Badge>
}
public #universal ChipError(props) {
    return <Badge {...props} variant="error">{props.children}</Badge>
}
