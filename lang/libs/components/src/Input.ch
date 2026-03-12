func input_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        padding: 0.75rem 0.9rem;
        border-radius: var(--chx-radius-sm);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        outline: none;
    }
    // TODO: transition not parsing with var
    // transition: border-color var(--chx-transition), box-shadow var(--chx-transition), background var(--chx-transition);
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
    }
    // TODO: transition not working with var
    // transition: border-color var(--chx-transition), box-shadow var(--chx-transition), background var(--chx-transition);
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
    }
    // TODO: transition not working with var
    // transition: border-color var(--chx-transition), box-shadow var(--chx-transition), background var(--chx-transition);
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
