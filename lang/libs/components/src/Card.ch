public #universal Card(props) {
    var card = style {
        position: relative;
        display: flex;
        flex-direction: column;
        gap: 0.375rem;
        border-radius: var(--radius-lg);
        border: 1px solid hsl(var(--border));
        background: hsl(var(--card));
        color: hsl(var(--card-foreground));
        box-shadow: var(--shadow-sm);
        transition: box-shadow 0.15s ease, border-color 0.15s ease;
        &:hover {
            border-color: hsl(var(--border));
        }
        &[data-interactive="true"] {
            cursor: pointer;
            &:hover {
                box-shadow: var(--shadow-md);
            }
        }
        &[data-size="sm"] {
            gap: 0.25rem;
            padding: 0;
            & > [data-slot="card-header"] { padding: 0.75rem 0.75rem 0; }
            & > [data-slot="card-content"] { padding: 0 0.75rem; }
            & > [data-slot="card-footer"] { padding: 0 0.75rem 0.75rem; }
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var size = props.size || "default"
    var out = classes + " " + card
    return <div data-interactive={props.onClick ? "true" : "false"} data-size={size != "default" ? size : null} class={out} onClick={props.onClick}>{props.children}</div>
}

public #universal CardHeader(props) {
    var card_header = style {
        display: flex;
        flex-direction: column;
        align-items: flex-start;
        gap: 0.25rem;
        padding: 1.25rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_header}>{props.children}</div>
}

public #universal CardTitle(props) {
    var card_title = style {
        font-size: 1.125rem;
        line-height: 1.5rem;
        font-weight: 600;
        letter-spacing: -0.01em;
        margin: 0;
        color: hsl(var(--foreground));
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var level = props.level || 3
    if(level == 2) { return <H2 class={classes + " " + card_title}>{props.children}</H2> }
    if(level == 4) { return <H4 class={classes + " " + card_title}>{props.children}</H4> }
    return <H3 class={classes + " " + card_title}>{props.children}</H3>
}

public #universal CardDescription(props) {
    var card_description = style {
        font-size: 0.875rem;
        line-height: 1.375rem;
        color: hsl(var(--muted-foreground));
        margin: 0;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <p class={classes + " " + card_description}>{props.children}</p>
}

public #universal CardMeta(props) {
    var card_meta = style {
        font-size: 0.75rem;
        line-height: 1.25rem;
        color: hsl(var(--muted-foreground));
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_meta}>{props.children}</div>
}

// Shadcn v2 header action slot — place a dropdown/button/icon action in the
// top-right of a CardHeader. Put it FIRST in the header so it sits on its own
// row above the title/description (the header stacks vertically).
public #universal CardAction(props) {
    var card_action = style {
        display: inline-flex;
        align-items: center;
        gap: 0.375rem;
        margin-left: auto;
        align-self: flex-end;
        padding-left: 1rem;
        margin-bottom: 0.25rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_action}>{props.children}</div>
}

public #universal CardContent(props) {
    var card_content = style {
        padding: 0 1.25rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_content}>{props.children}</div>
}

public #universal CardBody(props) {
    var card_body = style {
        display: flex;
        flex-direction: column;
        gap: 1rem;
        padding: 1.25rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_body}>{props.children}</div>
}

public #universal CardFooter(props) {
    var card_footer = style {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.5rem;
        padding: 0 1.25rem 1.25rem 1.25rem;
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div class={classes + " " + card_footer}>{props.children}</div>
}
