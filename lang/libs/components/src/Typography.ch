func h1_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 2.25rem;
        line-height: 2.5rem;
        font-weight: 800;
        letter-spacing: -0.025em;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func h2_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.875rem;
        line-height: 2.25rem;
        font-weight: 700;
        letter-spacing: -0.025em;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func h3_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.5rem;
        line-height: 2rem;
        font-weight: 600;
        letter-spacing: -0.025em;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func h4_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.25rem;
        line-height: 1.75rem;
        font-weight: 600;
        letter-spacing: -0.02em;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func h5_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.125rem;
        line-height: 1.75rem;
        font-weight: 600;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func h6_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1rem;
        line-height: 1.5rem;
        font-weight: 600;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func text_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1rem;
        line-height: 1.75rem;
        margin: 0;
        color: hsl(var(--chx-foreground));
        &[data-muted="true"] {
            color: hsl(var(--chx-muted-foreground));
        }
    }
}

func lead_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.25rem;
        line-height: 1.75rem;
        margin: 0;
        color: hsl(var(--chx-foreground));
    }
}

func caption_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.875rem;
        line-height: 1.25rem;
        color: hsl(var(--chx-muted-foreground));
        margin: 0;
    }
}

func code_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-family: var(--chx-font-mono);
        font-size: 0.875em;
        background: hsl(var(--chx-muted));
        color: hsl(var(--chx-foreground));
        border-radius: 0.25rem;
        padding: 0.2em 0.4em;
    }
}

func link_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: hsl(var(--chx-primary));
        text-decoration: underline;
        text-underline-offset: 4px;
        cursor: pointer;
        background: none;
        border: none;
        padding: 0;
        font: inherit;
        &:hover {
            text-decoration-thickness: 2px;
        }
    }
}

func blockquote_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        border-left: 3px solid hsl(var(--chx-border));
        padding-left: 1rem;
        color: hsl(var(--chx-muted-foreground));
        font-style: italic;
        .chx-blockquote-cite {
            display: block;
            margin-top: 0.5rem;
            font-size: 0.875rem;
            color: hsl(var(--chx-muted-foreground));
            font-style: normal;
        }
    }
}

public #universal H1(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h1 class={classes + " " + ${h1_styles(page)}}>{props.children}</h1>
}

public #universal H2(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h2 class={classes + " " + ${h2_styles(page)}}>{props.children}</h2>
}

public #universal H3(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h3 class={classes + " " + ${h3_styles(page)}}>{props.children}</h3>
}

public #universal H4(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h4 class={classes + " " + ${h4_styles(page)}}>{props.children}</h4>
}

public #universal H5(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h5 class={classes + " " + ${h5_styles(page)}}>{props.children}</h5>
}

public #universal H6(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <h6 class={classes + " " + ${h6_styles(page)}}>{props.children}</h6>
}

public #universal Heading(props) {
    var level = props.level || 1
    if(level == 2) { return <H2 {...props}>{props.children}</H2> }
    if(level == 3) { return <H3 {...props}>{props.children}</H3> }
    if(level == 4) { return <H4 {...props}>{props.children}</H4> }
    if(level == 5) { return <H5 {...props}>{props.children}</H5> }
    if(level == 6) { return <H6 {...props}>{props.children}</H6> }
    return <H1 {...props}>{props.children}</H1>
}

public #universal Text(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var tag = props.as || "p"
    var muted = props.muted || false
    var out = classes + " " + ${text_styles(page)}
    if(tag == "span") { return <span data-muted={muted ? "true" : "false"} class={out}>{props.children}</span> }
    if(tag == "div") { return <div data-muted={muted ? "true" : "false"} class={out}>{props.children}</div> }
    return <p data-muted={muted ? "true" : "false"} class={out}>{props.children}</p>
}

public #universal Lead(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <p class={classes + " " + ${lead_styles(page)}}>{props.children}</p>
}

public #universal Caption(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <p class={classes + " " + ${caption_styles(page)}}>{props.children}</p>
}

public #universal CodeText(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <code class={classes + " " + ${code_styles(page)}}>{props.children}</code>
}

public #universal Link(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var out = classes + " " + ${link_styles(page)}
    return <a class={out} href={props.href} target={props.target} rel={props.rel} id={props.id} onClick={props.onClick}>{props.children}</a>
}

public #universal Blockquote(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var cite = props.cite || ""
    return <blockquote class={classes + " " + ${blockquote_styles(page)}}>{props.children}{cite ? <cite class="chx-blockquote-cite">{cite}</cite> : null}</blockquote>
}

// Legacy aliases
public #universal TypographyH1(props) { return <H1 {...props} /> }
public #universal TypographyH2(props) { return <H2 {...props} /> }
public #universal TypographyH3(props) { return <H3 {...props} /> }
public #universal TypographyH4(props) { return <H4 {...props} /> }
public #universal TypographyH5(props) { return <H5 {...props} /> }
public #universal TypographyH6(props) { return <H6 {...props} /> }
public #universal TypographyText(props) { return <Text {...props}>{props.children}</Text> }
public #universal TypographyLead(props) { return <Lead {...props}>{props.children}</Lead> }
public #universal TypographyCaption(props) { return <Caption {...props}>{props.children}</Caption> }
public #universal TypographyCode(props) { return <CodeText {...props}>{props.children}</CodeText> }
