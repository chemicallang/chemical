// Shadcn-style Separator: a thin divider line.
//
// Props:
//   orientation  "horizontal" (default) | "vertical"
//   className    merged with the generated style class
//
// Horizontal separators stretch full width; vertical ones need an explicit
// height from the parent (set `style="height:..."` or wrap in a flex/stack
// with a set height).

func separator_styles(page : &mut HtmlPage) : *char {
    return #css {
        flex-shrink: 0;
        background: hsl(var(--border));
        &[data-orientation="horizontal"] {
            height: 1px;
            width: 100%;
        }
        &[data-orientation="vertical"] {
            width: 1px;
            height: auto;
            align-self: stretch;
        }
    }
}

public #universal Separator(props) {
    var orientation = props.orientation || "horizontal"
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div role="separator" aria-orientation={orientation} data-orientation={orientation} class={classes + " " + ${separator_styles(page)}} />
}
