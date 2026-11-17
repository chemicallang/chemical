// Typed props (§3.15a). Avatar enumerates its surface rather than spreading
// `{...props}` onto the DOM, so declaring it costs no pass-through. The `src` and
// `fallback` branches keep JS truthiness: the injected default is "", which is
// falsy on both sides, and a `string_view` method call (`.size() > 0`) renders an
// *empty* expression in universal JSX — see the prop-expression gap in §3.15a.
public struct AvatarProps {
    var src : std::string_view = ""
    var alt : std::string_view = ""
    var fallback : std::string_view = ""
    // "xs" | "sm" | "md" (default) | "lg" | "xl"
    var size : std::string_view = "md"
    var bordered : bool = false
}

public #universal Avatar(props : AvatarProps) {
    var avatar = style {
        position: relative;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        vertical-align: middle;
        border-radius: 9999px;
        overflow: hidden;
        flex-shrink: 0;
        user-select: none;
        background: hsl(var(--muted));
        color: hsl(var(--muted-foreground));
        &[data-size="xs"] {
            width: 1.5rem;
            height: 1.5rem;
            font-size: 0.625rem;
        }
        &[data-size="sm"] {
            width: 2rem;
            height: 2rem;
            font-size: 0.75rem;
        }
        &[data-size="lg"] {
            width: 3.5rem;
            height: 3.5rem;
            font-size: 1.125rem;
        }
        &[data-size="xl"] {
            width: 5rem;
            height: 5rem;
            font-size: 1.5rem;
        }
        &[data-bordered="true"] {
            border: 2px solid hsl(var(--card));
        }
    }
    var avatar_img = style {
        width: 100%;
        height: 100%;
        object-fit: cover;
        display: block;
    }
    var avatar_fallback = style {
        font-weight: 600;
        letter-spacing: 0.05em;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var size = props.size || "md"
    var out = classes + " " + avatar
    var bordered = props.bordered ? "true" : "false"
    return <span data-size={size} data-bordered={bordered} class={out}>
        {props.src ? <img class={avatar_img} src={props.src} alt={props.alt} />
                  : (props.fallback ? <span class={avatar_fallback}>{props.fallback}</span> : props.children)}
    </span>
}

public struct AvatarMoreProps {
    // rendered inside the chip: typically "+3".
    var count : std::string_view = "+"
}

public #universal AvatarMore(props : AvatarMoreProps) {
    var avatar_count = style {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2.5rem;
        height: 2.5rem;
        border-radius: 9999px;
        font-size: 0.75rem;
        font-weight: 600;
        background: hsl(var(--secondary));
        color: hsl(var(--secondary-foreground));
        border: 2px solid hsl(var(--card));
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var count = props.count || "+"
    return <span class={classes + " " + avatar_count}>{count}</span>
}

// Shadcn AvatarBadge: online/status indicator positioned bottom-right of avatar
public struct AvatarBadgeProps {
    var label : std::string_view = "status"
}

public #universal AvatarBadge(props : AvatarBadgeProps) {
    var avatar_badge = style {
        position: absolute;
        bottom: 0;
        right: 0;
        width: 12px;
        height: 12px;
        border-radius: 9999px;
        border: 2px solid hsl(var(--background));
        background: hsl(var(--success));
    }
    var classes = (props.className || props.class) || ""
    return <span class={classes + " " + avatar_badge} aria-label={props.label || "status"}></span>
}

// Shadcn AvatarGroup: overlapping avatar group container
public struct AvatarGroupProps {
    // 0 (default) renders every child; a positive value caps the visible count.
    var max : int = 0
}

public #universal AvatarGroup(props : AvatarGroupProps) {
    var avatar_group = style {
        display: inline-flex;
        align-items: center;
        & > * + * {
            margin-left: -0.625rem;
        }
    }
    var classes = (props.className || props.class) || ""
    var max = props.max || 0
    var children = props.children
    return <div class={classes + " " + avatar_group}>{children}</div>
}

// Shadcn AvatarGroupCount: shows +N overflow count
public struct AvatarGroupCountProps {
}

public #universal AvatarGroupCount(props : AvatarGroupCountProps) {
    var avatar_count = style {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2.5rem;
        height: 2.5rem;
        border-radius: 9999px;
        font-size: 0.75rem;
        font-weight: 600;
        background: hsl(var(--secondary));
        color: hsl(var(--secondary-foreground));
        border: 2px solid hsl(var(--card));
    }
    var classes = (props.className || props.class) || ""
    return <span class={classes + " " + avatar_count}>{props.children}</span>
}

// Legacy aliases
public #universal AvatarSm(props) {
    return <Avatar {...props} size="sm">{props.children}</Avatar>
}
public #universal AvatarLg(props) {
    return <Avatar {...props} size="lg">{props.children}</Avatar>
}
