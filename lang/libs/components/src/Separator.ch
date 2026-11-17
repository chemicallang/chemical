// Shadcn-style Separator: a thin divider line.
//
// Props:
//   orientation  "horizontal" (default) | "vertical"
//   className    merged with the generated style class
//
// Horizontal separators stretch full width; vertical ones need an explicit
// height from the parent (set `style="height:..."` or wrap in a flex/stack
// with a set height).

// Typed props (§3.15a): the component enumerates its surface instead of spreading
// `{...props}`, so a typo is a compile error. The default is the fallback the body
// already used, so existing call sites compile unchanged.
public struct SeparatorProps {
    // "horizontal" | "vertical"
    var orientation : std::string_view = "horizontal"
}

public #universal Separator(props : SeparatorProps) {
    var separator = style {
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
    var orientation = props.orientation || "horizontal"
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div role="separator" aria-orientation={orientation} data-orientation={orientation} class={classes + " " + separator} />
}
