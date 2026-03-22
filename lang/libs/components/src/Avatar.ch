func avatar_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 44px;
        height: 44px;
        border-radius: 50%;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        font-weight: 700;
        color: var(--chx-primary-fg);
        border: 2px solid var(--chx-border);
        box-shadow: var(--chx-shadow-sm);
        background: linear-gradient(135deg, var(--chx-accent), var(--chx-primary));
    }
}

func avatar_sm_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 32px;
        height: 32px;
        font-size: 0.75rem;
    }
}

func avatar_lg_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 64px;
        height: 64px;
        font-size: 1.1rem;
    }
}

public #universal Avatar(props) {
    return <div {...props} class={${avatar_styles(page)}}>{props.children}</div>
}

public #universal AvatarSm(props) {
    return <Avatar {...props} class={${avatar_sm_styles(page)}}>{props.children}</Avatar>
}

public #universal AvatarLg(props) {
    return <Avatar {...props} class={${avatar_lg_styles(page)}}>{props.children}</Avatar>
}
