func divider_styles(page : &mut HtmlPage) : *char {
    return #css {
        height: 1px;
        background: hsl(var(--border));
        border: 0;
        margin: 1.5rem 0;
    }
}

func kbd_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 0.2rem 0.45rem;
        border-radius: 6px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--muted));
        color: hsl(var(--foreground));
        font-size: 0.75rem;
        font-weight: 700;
        text-transform: uppercase;
        font-family: var(--font-mono);
    }
}

func skeleton_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-block;
        border-radius: var(--radius);
        background: hsl(var(--muted));
        position: relative;
        overflow: hidden;
        &::after {
            content: "";
            position: absolute;
            inset: 0;
            background: linear-gradient(90deg, transparent, hsl(var(--muted-foreground) / 0.08), transparent);
            background-size: 400px 100%;
            animation: chx-skeleton-shimmer 1.6s infinite linear;
        }
    }
}

func spinner_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-block;
        border-radius: 50%;
        border: 2px solid hsl(var(--border));
        border-top-color: hsl(var(--primary));
        animation: chx-spinner-rotate 0.8s linear infinite;
    }
}

func container_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        margin-inline: auto;
        padding-inline: 1rem;
        max-width: 80rem;
        box-sizing: border-box;
    }
}

func stack_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        box-sizing: border-box;
    }
}

func grid_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 1rem;
        box-sizing: border-box;
    }
}

func breadcrumbs_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        flex-wrap: wrap;
        gap: 0.375rem;
        font-size: 0.875rem;
        line-height: 1.25rem;
        color: hsl(var(--muted-foreground));
        list-style: none;
        padding: 0;
        margin: 0;
        & > * + * {
            margin-left: 0.375rem;
        }
    }
}

func breadcrumb_sep_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: hsl(var(--muted-foreground));
        opacity: 0.6;
        margin: 0 0.125rem;
    }
}

func breadcrumb_link_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: hsl(var(--muted-foreground));
        text-decoration: none;
        &:hover {
            color: hsl(var(--foreground));
            text-decoration: underline;
            text-underline-offset: 4px;
        }
    }
}

func breadcrumb_current_styles(page : &mut HtmlPage) : *char {
    return #css {
        color: hsl(var(--foreground));
        font-weight: 500;
    }
}

func breadcrumb_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.375rem;
    }
}

public #universal Divider(props) {
    return <hr {...props} class={${divider_styles(page)}} />
}

public #universal Kbd(props) {
    return <kbd {...props} class={${kbd_styles(page)}}>{props.children}</kbd>
}

public #universal Skeleton(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var width = props.width || ""
    var height = props.height || ""
    var circle = props.circle || false
    var out = classes + " " + ${skeleton_styles(page)}
    if(circle) { out = out + " chx-skeleton-circle" }
    var style = ""
    if(props.width) { style = style + "width:" + width + ";" }
    if(props.height) { style = style + "height:" + height + ";" }
    if(circle) {
        if(!props.width) { style = style + "width:2.5rem;" }
        if(!props.height) { style = style + "height:2.5rem;" }
        style = style + "border-radius:50%;"
    }
    return <span class={out} style={style} />
}

public #universal Spinner(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var size = props.size || "md"
    var out = classes + " " + ${spinner_styles(page)}
    var style = ""
    if(size == "sm") { style = "width:1rem;height:1rem;border-width:2px;" }
    else if(size == "lg") { style = "width:2.5rem;height:2.5rem;border-width:3px;" }
    else { style = "width:1.5rem;height:1.5rem;border-width:2px;" }
    return <span class={out} style={style} role="status" aria-label={props.label || "loading"} />
}

public #universal Container(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var size = props.size || "default"
    var out = classes + " " + ${container_styles(page)}
    if(size == "sm") { out = out + " chx-container-sm" }
    else if(size == "md") { out = out + " chx-container-md" }
    else if(size == "lg") { out = out + " chx-container-lg" }
    else if(size == "full") { out = out + " chx-container-full" }
    return <div class={out}>{props.children}</div>
}

public #universal Stack(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var direction = props.direction || "column"
    var gap = props.gap || "md"
    var out = classes + " " + ${stack_styles(page)}
    if(direction == "row") { out = out + " chx-stack-row" }
    else if(direction == "row-reverse") { out = out + " chx-stack-row-reverse" }
    else if(direction == "column-reverse") { out = out + " chx-stack-column-reverse" }
    else { out = out + " chx-stack-column" }
    if(gap == "none") { out = out + " chx-stack-gap-none" }
    else if(gap == "xs") { out = out + " chx-stack-gap-xs" }
    else if(gap == "sm") { out = out + " chx-stack-gap-sm" }
    else if(gap == "lg") { out = out + " chx-stack-gap-lg" }
    else if(gap == "xl") { out = out + " chx-stack-gap-xl" }
    else { out = out + " chx-stack-gap-md" }
    if(props.align) {
        if(props.align == "start") { out = out + " chx-stack-align-start" }
        else if(props.align == "center") { out = out + " chx-stack-align-center" }
        else if(props.align == "end") { out = out + " chx-stack-align-end" }
        else if(props.align == "stretch") { out = out + " chx-stack-align-stretch" }
    }
    if(props.justify) {
        if(props.justify == "start") { out = out + " chx-stack-justify-start" }
        else if(props.justify == "center") { out = out + " chx-stack-justify-center" }
        else if(props.justify == "end") { out = out + " chx-stack-justify-end" }
        else if(props.justify == "between") { out = out + " chx-stack-justify-between" }
    }
    return <div class={out}>{props.children}</div>
}

public #universal Grid(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var cols = props.cols || ""
    var gap = props.gap || "md"
    var out = classes + " " + ${grid_styles(page)}
    if(gap == "none") { out = out + " chx-grid-gap-none" }
    else if(gap == "xs") { out = out + " chx-grid-gap-xs" }
    else if(gap == "sm") { out = out + " chx-grid-gap-sm" }
    else if(gap == "lg") { out = out + " chx-grid-gap-lg" }
    else { out = out + " chx-grid-gap-md" }
    var style = ""
    if(props.cols) { style = style + "grid-template-columns:repeat(" + cols + ", minmax(0, 1fr));" }
    return <div class={out} style={style}>{props.children}</div>
}

public #universal Breadcrumbs(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <nav aria-label="Breadcrumb" class={classes + " " + ${breadcrumbs_styles(page)}}>{props.children}</nav>
}

public #universal BreadcrumbItem(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <li class={classes + " " + ${breadcrumb_item_styles(page)}}>{props.children}</li>
}

public #universal BreadcrumbLink(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <a href={props.href || "#"} class={classes + " " + ${breadcrumb_link_styles(page)}}>{props.children}</a>
}

public #universal BreadcrumbCurrent(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <span class={classes + " " + ${breadcrumb_current_styles(page)}} aria-current="page">{props.children}</span>
}

public #universal BreadcrumbSeparator(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var sep = props.separator || "/"
    return <span class={classes + " " + ${breadcrumb_sep_styles(page)}} aria-hidden="true">{sep}</span>
}
