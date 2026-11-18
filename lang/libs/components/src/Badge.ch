// Wrapper surfaces (Phase 2): each delegates to `<Badge {...props} variant="…">`.
// `variant` is deliberately absent from the wrapper struct — an injected default would
// be forwarded by the spread and fight the pinned attribute (see Button.ch).

public struct BadgeWrapperProps {
    var size : std::string_view = ""
}

// Typed props (Phase 2). Keeps `{...props}`? No — Badge renders a single <span>
// and forwards nothing, so its surface is exactly this.
public struct BadgeProps {
    // "default" | "secondary" | "accent" | "success" | "error" | "warning" | "info"
    // | "outline" | "outline-secondary" | "outline-accent" | "outline-success"
    // | "outline-error" | "outline-warning" | "outline-info"
    var variant : std::string_view = "default"
    // "xs" | "sm" | "md" (default) | "lg"
    var size : std::string_view = "md"
}

public #universal Badge(props : BadgeProps) {
    var badge = style {
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
        background: hsl(var(--muted));
        color: hsl(var(--muted-foreground));
        &[data-variant="secondary"] {
            background: hsl(var(--secondary));
            color: hsl(var(--secondary-foreground));
        }
        &[data-variant="accent"] {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &[data-variant="success"] {
            background: hsl(var(--success));
            color: hsl(var(--success-foreground));
        }
        &[data-variant="error"] {
            background: hsl(var(--destructive));
            color: hsl(var(--destructive-foreground));
        }
        &[data-variant="warning"] {
            background: hsl(var(--warning));
            color: hsl(var(--warning-foreground));
        }
        &[data-variant="info"] {
            background: hsl(var(--info));
            color: hsl(var(--info-foreground));
        }
        &[data-variant="outline"] {
            border-color: hsl(var(--border));
            color: hsl(var(--foreground));
            background: transparent;
        }
        &[data-variant="outline-secondary"] {
            border-color: hsl(var(--secondary));
            color: hsl(var(--secondary-foreground));
            background: transparent;
        }
        &[data-variant="outline-accent"] {
            border-color: hsl(var(--accent));
            color: hsl(var(--accent));
            background: transparent;
        }
        &[data-variant="outline-success"] {
            border-color: hsl(var(--success));
            color: hsl(var(--success));
            background: transparent;
        }
        &[data-variant="outline-error"] {
            border-color: hsl(var(--destructive));
            color: hsl(var(--destructive));
            background: transparent;
        }
        &[data-variant="outline-warning"] {
            border-color: hsl(var(--warning));
            color: hsl(var(--warning));
            background: transparent;
        }
        &[data-variant="outline-info"] {
            border-color: hsl(var(--info));
            color: hsl(var(--info));
            background: transparent;
        }
        &[data-variant="ghost"] {
            background: transparent;
            border-color: transparent;
            color: hsl(var(--foreground));
            &:hover { background: hsl(var(--accent)); color: hsl(var(--accent-foreground)); }
        }
        &[data-variant="link"] {
            background: transparent;
            border-color: transparent;
            color: hsl(var(--primary));
            text-decoration: underline;
            text-underline-offset: 4px;
            cursor: pointer;
            &:hover { text-decoration: underline; }
        }
        &[data-size="xs"] {
            font-size: 0.625rem;
            padding: 0 0.4375rem;
            line-height: 1.125rem;
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
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var variant = props.variant || "default"
    var size = props.size || "md"
    var out = classes + " " + badge
    return <span data-variant={variant} data-size={size} class={out}>{props.children}</span>
}

// Legacy aliases
public #universal BadgeAccent(props : BadgeWrapperProps) {
    return <Badge {...props} variant="accent">{props.children}</Badge>
}
public #universal BadgeSuccess(props : BadgeWrapperProps) {
    return <Badge {...props} variant="success">{props.children}</Badge>
}
public #universal BadgeError(props : BadgeWrapperProps) {
    return <Badge {...props} variant="error">{props.children}</Badge>
}
public #universal Chip(props : BadgeWrapperProps) {
    return <Badge {...props} variant="outline">{props.children}</Badge>
}
public #universal ChipAccent(props : BadgeWrapperProps) {
    return <Badge {...props} variant="accent">{props.children}</Badge>
}
public #universal ChipSuccess(props : BadgeWrapperProps) {
    return <Badge {...props} variant="success">{props.children}</Badge>
}
public #universal ChipError(props : BadgeWrapperProps) {
    return <Badge {...props} variant="error">{props.children}</Badge>
}
public #universal ChipWarning(props : BadgeWrapperProps) {
    return <Badge {...props} variant="warning">{props.children}</Badge>
}
public #universal ChipInfo(props : BadgeWrapperProps) {
    return <Badge {...props} variant="info">{props.children}</Badge>
}
public #universal BadgeOutline(props : BadgeWrapperProps) {
    return <Badge {...props} variant="outline">{props.children}</Badge>
}
public #universal BadgeInfo(props : BadgeWrapperProps) {
    return <Badge {...props} variant="info">{props.children}</Badge>
}
public #universal BadgeWarning(props : BadgeWrapperProps) {
    return <Badge {...props} variant="warning">{props.children}</Badge>
}
public #universal BadgeGhost(props : BadgeWrapperProps) {
    return <Badge {...props} variant="ghost">{props.children}</Badge>
}
public #universal BadgeLink(props : BadgeWrapperProps) {
    return <Badge {...props} variant="link">{props.children}</Badge>
}
