func typo_h1_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 3rem;
        line-height: 1.04;
        letter-spacing: -0.04em;
        font-weight: 800;
        margin: 0;
        color: var(--chx-text-main);
    }
}

func typo_h2_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 2.2rem;
        line-height: 1.08;
        letter-spacing: -0.03em;
        font-weight: 760;
        margin: 0;
        color: var(--chx-text-main);
    }
}

func typo_h3_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.35rem;
        line-height: 1.15;
        font-weight: 720;
        margin: 0;
        color: var(--chx-text-main);
    }
}

func typo_text_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        line-height: 1.65;
        color: var(--chx-text-main);
    }
}

func typo_lead_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        font-size: 1.08rem;
        line-height: 1.75;
        color: var(--chx-text-muted);
    }
}

func typo_caption_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        font-size: 0.82rem;
        letter-spacing: 0.02em;
        color: var(--chx-text-muted);
    }
}

func typo_code_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        padding: 0.15rem 0.4rem;
        border-radius: 8px;
        border: 1px solid var(--chx-border);
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-size: 0.82rem;
        font-family: ui-monospace, "SFMono-Regular", monospace;
    }
}

func link_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: var(--chx-accent);
        text-decoration: none;
        font-weight: 600;
        transition: color 0.18s ease, opacity 0.18s ease;
        &:hover {
            opacity: 0.82;
            text-decoration: underline;
        }
    }
}

public #universal H1(props) {
    return <h1 {...props} class={${typo_h1_styles(page)}}>{props.children}</h1>
}

public #universal H2(props) {
    return <h2 {...props} class={${typo_h2_styles(page)}}>{props.children}</h2>
}

public #universal H3(props) {
    return <h3 {...props} class={${typo_h3_styles(page)}}>{props.children}</h3>
}

public #universal Text(props) {
    return <p {...props} class={${typo_text_styles(page)}}>{props.children}</p>
}

public #universal Lead(props) {
    return <p {...props} class={${typo_lead_styles(page)}}>{props.children}</p>
}

public #universal Caption(props) {
    return <p {...props} class={${typo_caption_styles(page)}}>{props.children}</p>
}

public #universal CodeText(props) {
    return <code {...props} class={${typo_code_styles(page)}}>{props.children}</code>
}

public #universal Link(props) {
    return <a {...props} class={${link_styles(page)}}>{props.children}</a>
}
