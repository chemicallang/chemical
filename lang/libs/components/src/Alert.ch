func alert_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: flex-start;
        gap: 0.75rem;
        padding: 1rem 1.2rem;
        border-radius: var(--chx-radius);
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        box-shadow: var(--chx-shadow-sm);
    }
}

func alert_accent_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-color: rgba(59, 130, 246, 0.4);
        background: rgba(59, 130, 246, 0.08);
    }
}

func alert_success_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-color: rgba(16, 185, 129, 0.4);
        background: rgba(16, 185, 129, 0.08);
    }
}

func alert_error_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-color: rgba(239, 68, 68, 0.4);
        background: rgba(239, 68, 68, 0.08);
    }
}

func alert_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-weight: 700;
        margin: 0 0 0.25rem 0;
    }
}

func alert_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: var(--chx-text-muted);
        margin: 0;
    }
}

public #universal Alert(props) {
    return <div {...props} class={${alert_styles(page)}}>{props.children}</div>
}

public #universal AlertAccent(props) {
    return <Alert {...props} class={${alert_accent_styles(page)}}>{props.children}</Alert>
}

public #universal AlertSuccess(props) {
    return <Alert {...props} class={${alert_success_styles(page)}}>{props.children}</Alert>
}

public #universal AlertError(props) {
    return <Alert {...props} class={${alert_error_styles(page)}}>{props.children}</Alert>
}

public #universal AlertTitle(props) {
    return <h4 {...props} class={${alert_title_styles(page)}}>{props.children}</h4>
}

public #universal AlertBody(props) {
    return <p {...props} class={${alert_body_styles(page)}}>{props.children}</p>
}
