func button_base_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.6rem;
        padding: 0.75rem 1.25rem;
        border-radius: var(--chx-radius);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        font-weight: 600;
        letter-spacing: -0.01em;
        box-shadow: var(--chx-shadow-sm);
        cursor: pointer;
        outline: none;
    }
    // TODO: transition not working with var
    // transition: transform var(--chx-transition), box-shadow var(--chx-transition), background var(--chx-transition), color var(--chx-transition), border-color var(--chx-transition);
}

func button_primary_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-primary);
        color: var(--chx-primary-fg);
        border-color: transparent;
        box-shadow: var(--chx-shadow-lg);
    }
}

func button_ghost_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: transparent;
        color: var(--chx-text-main);
        border-color: transparent;
        box-shadow: none;
    }
}

func button_outline_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: transparent;
        color: var(--chx-text-main);
        border-color: var(--chx-border-strong);
        box-shadow: none;
    }
}

func button_danger_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-error);
        color: #fff;
        border-color: transparent;
    }
}

func button_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-success);
        color: #fff;
        border-color: transparent;
    }
}

func button_sm_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.5rem 0.85rem;
        font-size: 0.85rem;
        border-radius: var(--chx-radius-sm);
    }
}

func button_lg_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.95rem 1.6rem;
        font-size: 1.05rem;
    }
    // TODO: calc not working
    // error: expected a length value
    // border-radius: calc(var(--chx-radius) + 4px);
}

func icon_button_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 44px;
        height: 44px;
        border-radius: 999px;
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        display: inline-flex;
        align-items: center;
        justify-content: center;
        box-shadow: var(--chx-shadow-sm);
        cursor: pointer;
        &:hover {
            background: var(--chx-surface-2);
            border-color: var(--chx-border-strong);
        }
    }
}

func fab_styles(page : &mut HtmlPage) : *char {
    return #css {
        min-width: 56px;
        height: 56px;
        padding: 0 1.2rem;
        border-radius: 999px;
        border: none;
        background: var(--chx-accent);
        color: white;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.65rem;
        font-weight: 700;
        box-shadow: var(--chx-shadow-lg);
        cursor: pointer;
        &:hover {
            transform: translateY(-1px);
        }
    }
}

public #universal Button(props) {
    return <button {...props} class={${button_base_styles(page)}}>{props.children}</button>
}

public #universal ButtonPrimary(props) {
    return <Button {...props} class={${button_primary_styles(page)}}>{props.children}</Button>
}

public #universal ButtonGhost(props) {
    return <Button {...props} class={${button_ghost_styles(page)}}>{props.children}</Button>
}

public #universal ButtonOutline(props) {
    return <Button {...props} class={${button_outline_styles(page)}}>{props.children}</Button>
}

public #universal ButtonDanger(props) {
    return <Button {...props} class={${button_danger_styles(page)}}>{props.children}</Button>
}

public #universal ButtonSuccess(props) {
    return <Button {...props} class={${button_success_styles(page)}}>{props.children}</Button>
}

public #universal ButtonSm(props) {
    return <Button {...props} class={${button_sm_styles(page)}}>{props.children}</Button>
}

public #universal ButtonLg(props) {
    return <Button {...props} class={${button_lg_styles(page)}}>{props.children}</Button>
}

public #universal IconButton(props) {
    return <button {...props} class={${icon_button_styles(page)}}>{props.children}</button>
}

public #universal Fab(props) {
    return <button {...props} class={${fab_styles(page)}}>{props.children}</button>
}
