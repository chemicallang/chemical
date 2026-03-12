func divider_styles(page : &mut HtmlPage) : *char {
    return #css {
        height: 1px;
        background: var(--chx-border);
        border: 0;
        margin: 1.5rem 0;
    }
}

func chip_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.4rem;
        padding: 0.4rem 0.7rem;
        border-radius: 999px;
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-size: 0.85rem;
        border: 1px solid var(--chx-border);
    }
}

func kbd_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 0.2rem 0.45rem;
        border-radius: 6px;
        border: 1px solid var(--chx-border-strong);
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-size: 0.75rem;
        font-weight: 700;
        text-transform: uppercase;
    }
}

public #universal Divider(props) {
    return <hr {...props} class={${divider_styles(page)}} />
}

public #universal Chip(props) {
    return <span {...props} class={${chip_styles(page)}}>{props.children}</span>
}

public #universal Kbd(props) {
    return <kbd {...props} class={${kbd_styles(page)}}>{props.children}</kbd>
}
