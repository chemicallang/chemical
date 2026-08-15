func avatar_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

func avatar_img_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        height: 100%;
        object-fit: cover;
        display: block;
    }
}

func avatar_fallback_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-weight: 600;
        letter-spacing: 0.05em;
    }
}

func avatar_group_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        & > * + * {
            margin-left: -0.625rem;
        }
    }
}

func avatar_count_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

public #universal Avatar(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var size = props.size || "md"
    var out = classes + " " + ${avatar_styles(page)}
    var bordered = props.bordered ? "true" : "false"
    return <span data-size={size} data-bordered={bordered} class={out}>{
        props.src ? <img class={${avatar_img_styles(page)}} src={props.src} alt={props.alt} />
                  : (props.fallback ? <span class={${avatar_fallback_styles(page)}}>{props.fallback}</span> : props.children)
    }</span>
}

public #universal AvatarGroup(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + ${avatar_group_styles(page)}}>{props.children}</div>
}

public #universal AvatarMore(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var count = props.count || "+"
    return <span class={classes + " " + ${avatar_count_styles(page)}}>{count}</span>
}

// Legacy aliases
public #universal AvatarSm(props) {
    return <Avatar {...props} size="sm">{props.children}</Avatar>
}
public #universal AvatarLg(props) {
    return <Avatar {...props} size="lg">{props.children}</Avatar>
}
