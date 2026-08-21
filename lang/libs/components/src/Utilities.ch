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
        &.chx-skeleton-circle {
            border-radius: 50%;
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
        &.chx-container-sm { max-width: 40rem; }
        &.chx-container-md { max-width: 48rem; }
        &.chx-container-lg { max-width: 64rem; }
        &.chx-container-full { max-width: none; padding-inline: 0; }
    }
}

func stack_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        box-sizing: border-box;
        &.chx-stack-column { flex-direction: column; }
        &.chx-stack-row { flex-direction: row; }
        &.chx-stack-row-reverse { flex-direction: row-reverse; }
        &.chx-stack-column-reverse { flex-direction: column-reverse; }
        &.chx-stack-gap-none { gap: 0; }
        &.chx-stack-gap-xs { gap: 0.25rem; }
        &.chx-stack-gap-sm { gap: 0.5rem; }
        &.chx-stack-gap-md { gap: 1rem; }
        &.chx-stack-gap-lg { gap: 1.5rem; }
        &.chx-stack-gap-xl { gap: 2rem; }
        &.chx-stack-align-start { align-items: flex-start; }
        &.chx-stack-align-center { align-items: center; }
        &.chx-stack-align-end { align-items: flex-end; }
        &.chx-stack-align-stretch { align-items: stretch; }
        &.chx-stack-justify-start { justify-content: flex-start; }
        &.chx-stack-justify-center { justify-content: center; }
        &.chx-stack-justify-end { justify-content: flex-end; }
        &.chx-stack-justify-between { justify-content: space-between; }
    }
}

func grid_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 1rem;
        box-sizing: border-box;
        &.chx-grid-gap-none { gap: 0; }
        &.chx-grid-gap-xs { gap: 0.25rem; }
        &.chx-grid-gap-sm { gap: 0.5rem; }
        &.chx-grid-gap-md { gap: 1rem; }
        &.chx-grid-gap-lg { gap: 1.5rem; }
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
    return <div class={out} style={style} aria-hidden="true" {...props} />
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

// BreadcrumbEllipsis: collapsed breadcrumb indicator
func breadcrumb_ellipsis_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 1.5rem;
        height: 1.5rem;
        border-radius: var(--radius);
        color: hsl(var(--muted-foreground));
        cursor: pointer;
        &:hover {
            background: hsl(var(--accent));
        }
    }
}

public #universal BreadcrumbEllipsis(props) {
    var classes = (props.className || props.class) || ""
    return <span class={classes + " " + ${breadcrumb_ellipsis_styles(page)}} role="button" aria-label={props.ariaLabel || "More"} {...props}>
        <svg width="15" height="15" viewBox="0 0 15 15" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M3.625 7.5C3.625 8.12132 3.12132 8.625 2.5 8.625C1.87868 8.625 1.375 8.12132 1.375 7.5C1.375 6.87868 1.87868 6.375 2.5 6.375C3.12132 6.375 3.625 6.87868 3.625 7.5ZM8.625 7.5C8.625 8.12132 8.12132 8.625 7.5 8.625C6.87868 8.625 6.375 8.12132 6.375 7.5C6.375 6.87868 6.87868 6.375 7.5 6.375C8.12132 6.375 8.625 6.87868 8.625 7.5ZM13.625 7.5C13.625 8.12132 13.1213 8.625 12.5 8.625C11.8787 8.625 11.375 8.12132 11.375 7.5C11.375 6.87868 11.8787 6.375 12.5 6.375C13.1213 6.375 13.625 6.87868 13.625 7.5Z" fill="currentColor" fill-rule="evenodd" clip-rule="evenodd"></path></svg>
    </span>
}

// BreadcrumbList: ordered list wrapper (shadcn composition)
public #universal BreadcrumbList(props) {
    var classes = (props.className || props.class) || ""
    return <ol class={classes + " " + ${breadcrumbs_styles(page)}}>{props.children}</ol>
}
