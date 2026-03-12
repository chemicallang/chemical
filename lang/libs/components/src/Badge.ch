func badge_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.4rem;
        padding: 0.35rem 0.7rem;
        border-radius: 999px;
        border: 1px solid var(--chx-border);
        background: var(--chx-surface-2);
        color: var(--chx-text-muted);
        font-size: 0.8rem;
        font-weight: 600;
        letter-spacing: 0.01em;
    }
}

func badge_accent_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: rgba(59, 130, 246, 0.15);
        color: var(--chx-accent);
        border-color: rgba(59, 130, 246, 0.3);
    }
}

func badge_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: rgba(16, 185, 129, 0.15);
        color: var(--chx-success);
        border-color: rgba(16, 185, 129, 0.3);
    }
}

func badge_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: rgba(239, 68, 68, 0.15);
        color: var(--chx-error);
        border-color: rgba(239, 68, 68, 0.3);
    }
}

public #universal Badge(props) {
    return <span {...props} class={${badge_styles(page)}}>{props.children}</span>
}

public #universal BadgeAccent(props) {
    return <Badge {...props} class={${badge_accent_styles(page)}}>{props.children}</Badge>
}

public #universal BadgeSuccess(props) {
    return <Badge {...props} class={${badge_success_styles(page)}}>{props.children}</Badge>
}

public #universal BadgeError(props) {
    return <Badge {...props} class={${badge_error_styles(page)}}>{props.children}</Badge>
}
