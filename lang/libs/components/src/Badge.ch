func badge_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-badge-bg: var(--chx-surface-2);
        --chx-badge-color: var(--chx-text-muted);
        --chx-badge-border: var(--chx-border);
        display: inline-flex;
        align-items: center;
        gap: 0.4rem;
        padding: 0.35rem 0.7rem;
        border-radius: 999px;
        border: 1px solid var(--chx-badge-border);
        background: var(--chx-badge-bg);
        color: var(--chx-badge-color);
        font-size: 0.8rem;
        font-weight: 600;
        letter-spacing: 0.01em;
    }
}

func badge_accent_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-badge-bg: rgba(59, 130, 246, 0.15);
        --chx-badge-color: var(--chx-accent);
        --chx-badge-border: rgba(59, 130, 246, 0.3);
    }
}

func badge_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-badge-bg: rgba(16, 185, 129, 0.15);
        --chx-badge-color: var(--chx-success);
        --chx-badge-border: rgba(16, 185, 129, 0.3);
    }
}

func badge_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-badge-bg: rgba(239, 68, 68, 0.15);
        --chx-badge-color: var(--chx-error);
        --chx-badge-border: rgba(239, 68, 68, 0.3);
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
