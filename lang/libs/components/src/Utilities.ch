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
        --chx-chip-bg: var(--chx-surface-2);
        --chx-chip-color: var(--chx-text-main);
        --chx-chip-border: var(--chx-border);
        display: inline-flex;
        align-items: center;
        gap: 0.4rem;
        padding: 0.4rem 0.7rem;
        border-radius: 999px;
        background: var(--chx-chip-bg);
        color: var(--chx-chip-color);
        font-size: 0.85rem;
        border: 1px solid var(--chx-chip-border);
    }
}

func chip_accent_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-chip-bg: rgba(59, 130, 246, 0.14);
        --chx-chip-color: var(--chx-accent);
        --chx-chip-border: rgba(59, 130, 246, 0.3);
    }
}

func chip_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-chip-bg: rgba(16, 185, 129, 0.14);
        --chx-chip-color: var(--chx-success);
        --chx-chip-border: rgba(16, 185, 129, 0.28);
    }
}

func chip_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        --chx-chip-bg: rgba(239, 68, 68, 0.14);
        --chx-chip-color: var(--chx-error);
        --chx-chip-border: rgba(239, 68, 68, 0.28);
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

public #universal ChipAccent(props) {
    return <Chip {...props} class={${chip_accent_styles(page)}}>{props.children}</Chip>
}

public #universal ChipSuccess(props) {
    return <Chip {...props} class={${chip_success_styles(page)}}>{props.children}</Chip>
}

public #universal ChipError(props) {
    return <Chip {...props} class={${chip_error_styles(page)}}>{props.children}</Chip>
}

public #universal Kbd(props) {
    return <kbd {...props} class={${kbd_styles(page)}}>{props.children}</kbd>
}
