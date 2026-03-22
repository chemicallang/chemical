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
        transition: transform 0.18s ease, background 0.18s ease, color 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
        &:hover {
            transform: translateY(-1px);
            border-color: var(--chx-border-strong);
            box-shadow: var(--chx-shadow);
        }
    }
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
        border-radius: calc(var(--chx-radius) + 4px);
    }
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
        padding: 0;
        transition: transform 0.18s ease, background 0.18s ease, border-color 0.18s ease, box-shadow 0.18s ease;
        & > * {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 100%;
            height: 100%;
            min-width: 0;
            min-height: 0;
            border: 0;
            border-radius: 0;
            background: transparent;
            box-shadow: none;
            line-height: 1;
            font-size: 1rem;
            font-family: ui-monospace, "SFMono-Regular", monospace;
            transform: translate(-0.02em, -0.04em);
        }
        & > * > * {
            width: 100%;
            height: 100%;
            border: 0;
            border-radius: 0;
            background: transparent;
            box-shadow: none;
            color: inherit;
            line-height: 1;
            font-size: 1rem;
            font-family: ui-monospace, "SFMono-Regular", monospace;
        }
        &:hover {
            background: var(--chx-surface-2);
            border-color: var(--chx-border-strong);
            transform: translateY(-1px);
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
        transition: transform 0.18s ease, box-shadow 0.18s ease, background 0.18s ease;
        & > * {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            border: 0;
            border-radius: 0;
            background: transparent;
            box-shadow: none;
            color: inherit;
            line-height: 1;
        }
        & > *:first-child {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 1rem;
            height: 1rem;
            min-width: 1rem;
            font-size: 1rem;
            font-family: ui-monospace, "SFMono-Regular", monospace;
            transform: translate(-0.02em, -0.04em);
        }
        & > *:first-child > * {
            width: 100%;
            height: 100%;
            border: 0;
            border-radius: 0;
            background: transparent;
            box-shadow: none;
            color: inherit;
            line-height: 1;
            font-size: 1rem;
            font-family: ui-monospace, "SFMono-Regular", monospace;
        }
        &:hover {
            transform: translateY(-1px);
            box-shadow: 0 16px 34px rgba(15, 23, 42, 0.22);
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
