func paper_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.03), transparent 55%), var(--chx-surface);
        border: 1px solid var(--chx-border);
        border-radius: var(--chx-radius);
        box-shadow: var(--chx-shadow);
        padding: 1.25rem;
    }
}

func appbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        padding: 0.9rem 1.15rem;
        border-radius: 18px;
        border: 1px solid var(--chx-border);
        background: rgba(15, 23, 42, 0.04);
        box-shadow: var(--chx-shadow-sm);
    }
}

func drawer_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.85rem;
        padding: 1.1rem;
        border: 1px solid var(--chx-border);
        border-radius: var(--chx-radius);
        background: var(--chx-surface);
        box-shadow: var(--chx-shadow);
    }
}

func menu_styles(page : &mut HtmlPage) : *char {
    return #css {
        min-width: 220px;
        display: grid;
        gap: 0.35rem;
        padding: 0.55rem;
        border: 1px solid var(--chx-border);
        border-radius: 14px;
        background: var(--chx-surface);
        box-shadow: var(--chx-shadow);
    }
}

func menu_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.7rem 0.85rem;
        border-radius: 10px;
        color: var(--chx-text-main);
        text-decoration: none;
        transition: background 0.18s ease, color 0.18s ease;
        &:hover {
            background: var(--chx-surface-2);
        }
    }
}

func popover_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.55rem;
        padding: 0.9rem 1rem;
        border: 1px solid var(--chx-border);
        border-radius: 14px;
        background: var(--chx-surface);
        box-shadow: var(--chx-shadow);
    }
}

func dialog_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 420px;
        max-width: 100%;
        margin: 0;
        border: 1px solid var(--chx-border);
        border-radius: 20px;
        padding: 1.15rem;
        background: var(--chx-surface);
        box-shadow: var(--chx-shadow-lg);
        color: var(--chx-text-main);
    }
}

func snackbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.8rem;
        padding: 0.85rem 1rem;
        border-radius: 999px;
        border: 1px solid var(--chx-border-strong);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        box-shadow: var(--chx-shadow);
    }
}

func tooltip_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        padding: 0.45rem 0.65rem;
        border-radius: 10px;
        border: 1px solid var(--chx-border);
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-size: 0.82rem;
        box-shadow: var(--chx-shadow-sm);
    }
}

func icon_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2rem;
        height: 2rem;
        border-radius: 999px;
        background: var(--chx-surface-2);
        border: 1px solid var(--chx-border);
        color: var(--chx-text-main);
        font-weight: 700;
    }
}

public #universal Paper(props) {
    return <section {...props} class={${paper_styles(page)}}>{props.children}</section>
}

public #universal AppBar(props) {
    return <header {...props} class={${appbar_styles(page)}}>{props.children}</header>
}

public #universal Drawer(props) {
    return <aside {...props} class={${drawer_styles(page)}}>{props.children}</aside>
}

public #universal Menu(props) {
    return <div {...props} class={${menu_styles(page)}}>{props.children}</div>
}

public #universal MenuItem(props) {
    return <a {...props} class={${menu_item_styles(page)}}>{props.children}</a>
}

public #universal Popover(props) {
    return <div {...props} class={${popover_styles(page)}}>{props.children}</div>
}

public #universal Dialog(props) {
    return <dialog {...props} open={props.open} class={${dialog_styles(page)}}>{props.children}</dialog>
}

public #universal Snackbar(props) {
    return <div {...props} role="status" class={${snackbar_styles(page)}}>{props.children}</div>
}

public #universal Tooltip(props) {
    return <span {...props} class={${tooltip_styles(page)}}>{props.children}</span>
}

public #universal Icon(props) {
    return <span {...props} class={${icon_styles(page)}}>{props.children}</span>
}
