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

func input_filled_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-surface-2);
        border-color: transparent;
    }
}

func input_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-color: rgba(16, 185, 129, 0.45);
        box-shadow: 0 0 0 4px rgba(16, 185, 129, 0.10);
    }
}

func input_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-color: rgba(239, 68, 68, 0.45);
        box-shadow: 0 0 0 4px rgba(239, 68, 68, 0.10);
    }
}

func input_ghost_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: transparent;
        border-color: transparent;
        border-bottom: 1px solid var(--chx-border-strong);
        border-radius: 0;
        padding-left: 0;
        padding-right: 0;
    }
}

func input_sm_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.55rem 0.75rem;
        font-size: 0.88rem;
    }
}

func input_lg_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.95rem 1rem;
        font-size: 1.02rem;
    }
}

func input_disabled_styles(page : &mut HtmlPage) : *char {
    return #css {
        opacity: 0.62;
        cursor: not-allowed;
        background: var(--chx-surface-2);
    }
}

func field_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.45rem;
    }
}

func field_label_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.9rem;
        font-weight: 650;
        color: var(--chx-text-main);
    }
}

func field_hint_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.82rem;
        color: var(--chx-text-muted);
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

public #universal InputFilled(props) {
    return <Input {...props} class={${input_filled_styles(page)}} />
}

public #universal InputSuccess(props) {
    return <Input {...props} class={${input_success_styles(page)}} />
}

public #universal InputError(props) {
    return <Input {...props} class={${input_error_styles(page)}} />
}

public #universal InputGhost(props) {
    return <Input {...props} class={${input_ghost_styles(page)}} />
}

public #universal InputSm(props) {
    return <Input {...props} class={${input_sm_styles(page)}} />
}

public #universal InputLg(props) {
    return <Input {...props} class={${input_lg_styles(page)}} />
}

public #universal InputDisabled(props) {
    return <Input {...props} disabled={true} class={${input_disabled_styles(page)}} />
}

public #universal Field(props) {
    return <label {...props} class={${field_styles(page)}}>{props.children}</label>
}

public #universal FieldLabel(props) {
    return <span {...props} class={${field_label_styles(page)}}>{props.children}</span>
}

public #universal FieldHint(props) {
    return <span {...props} class={${field_hint_styles(page)}}>{props.children}</span>
}
