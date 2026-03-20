func input_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        padding: 0.75rem 0.9rem;
        border-radius: var(--chx-radius-sm);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        outline: none;
        transition: border-color 0.18s ease, box-shadow 0.18s ease, background 0.18s ease;
        &:hover {
            border-color: var(--chx-border-strong);
        }
        &:focus {
            border-color: var(--chx-accent);
            box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.14);
        }
    }
}

func textarea_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        min-height: 120px;
        padding: 0.85rem 0.9rem;
        border-radius: var(--chx-radius-sm);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        outline: none;
        resize: vertical;
        transition: border-color 0.18s ease, box-shadow 0.18s ease, background 0.18s ease;
        &:hover {
            border-color: var(--chx-border-strong);
        }
        &:focus {
            border-color: var(--chx-accent);
            box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.14);
        }
    }
}

func select_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        padding: 0.75rem 0.9rem;
        border-radius: var(--chx-radius-sm);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        outline: none;
        transition: border-color 0.18s ease, box-shadow 0.18s ease, background 0.18s ease;
        &:hover {
            border-color: var(--chx-border-strong);
        }
        &:focus {
            border-color: var(--chx-accent);
            box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.14);
        }
    }
}

public #universal Input(props) {
    return <input {...props} class={${input_styles(page)}} />
}

public #universal TextArea(props) {
    return <textarea {...props} class={${textarea_styles(page)}}>{props.children}</textarea>
}

public #universal Select(props) {
    return <select {...props} class={${select_styles(page)}}>{props.children}</select>
}
