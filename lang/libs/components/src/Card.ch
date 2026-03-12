func card_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-surface);
        border: 1px solid var(--chx-border);
        border-radius: var(--chx-radius);
        padding: 1.5rem;
        box-shadow: var(--chx-shadow);
    }
}

func card_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        margin-bottom: 1rem;
    }
}

func card_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.2rem;
        font-weight: 700;
        color: var(--chx-text-main);
        margin: 0;
    }
}

func card_meta_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.9rem;
        color: var(--chx-text-muted);
        margin: 0;
    }
}

func card_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: var(--chx-text-main);
    }
}

func card_footer_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.75rem;
        margin-top: 1.25rem;
    }
}

public #universal Card(props) {
    return <div {...props} class={${card_styles(page)}}>{props.children}</div>
}

public #universal CardHeader(props) {
    return <div {...props} class={${card_header_styles(page)}}>{props.children}</div>
}

public #universal CardTitle(props) {
    return <h3 {...props} class={${card_title_styles(page)}}>{props.children}</h3>
}

public #universal CardMeta(props) {
    return <p {...props} class={${card_meta_styles(page)}}>{props.children}</p>
}

public #universal CardBody(props) {
    return <div {...props} class={${card_body_styles(page)}}>{props.children}</div>
}

public #universal CardFooter(props) {
    return <div {...props} class={${card_footer_styles(page)}}>{props.children}</div>
}
