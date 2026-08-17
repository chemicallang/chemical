// Shadcn-style ToggleGroup: a group of toggle buttons built from children:
//
//      <ToggleGroup type="single" defaultValue="bold" onValueChange={(v) => ...}>
//          <ToggleGroupItem value="bold">Bold</ToggleGroupItem>
//          <ToggleGroupItem value="italic">Italic</ToggleGroupItem>
//      </ToggleGroup>
//
//      <ToggleGroup type="multiple" defaultValue={["bold"]}>
//          <ToggleGroupItem value="bold">Bold</ToggleGroupItem>
//      </ToggleGroup>
//
// Props:
//   type           "single" (default, one pressed at a time) | "multiple"
//   value          controlled selection; a string (single) or array (multiple)
//   defaultValue   uncontrolled initial selection; string or array
//   onValueChange  fired with the new selection (string or array)
//   variant        "default" | "outline" | "ghost" (default "default")
//   size           "sm" | "default" | "lg" (default "default")
//   disabled       locks all items
//   className      merged with the group class
//
// The group owns selection state via context (keyed by `name`, defaulting to
// "default"). In single mode the selection is a string; in multiple mode it is
// an array. SSR renders items unpressed (children render before the provider's
// SSR function); hydration applies the selection.

func toggle_group_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        flex-wrap: wrap;
        align-items: center;
        gap: 0.25rem;
        border-radius: var(--radius-md);
        &[data-disabled="true"] {
            opacity: 0.55;
        }
    }
}

func toggle_group_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 0.375rem;
        height: 2.25rem;
        padding: 0 0.75rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid transparent;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-size: 0.8125rem;
        font-weight: 500;
        cursor: pointer;
        user-select: none;
        transition: background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease, box-shadow 0.15s ease;
        &:hover {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
        &[data-pressed="true"] {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &[data-variant="outline"] {
            border-color: hsl(var(--border));
            &[data-pressed="true"] {
                background: hsl(var(--accent));
                border-color: hsl(var(--border));
            }
        }
        &[data-variant="ghost"] {
            &[data-pressed="true"] {
                background: hsl(var(--accent));
            }
        }
        &[data-size="sm"] {
            height: 1.875rem;
            padding: 0 0.625rem;
            font-size: 0.75rem;
        }
        &[data-size="lg"] {
            height: 2.625rem;
            padding: 0 1rem;
            font-size: 0.9375rem;
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            pointer-events: none;
        }
    }
}

public #universal ToggleGroup(props) {
    var multiple = props.type == "multiple"
    state selected = props.defaultValue || ""
    const ctx = createContext("tg-" + (props.name || "default"), "")
    var disabled = props.disabled || false
    var variant = props.variant || "default"
    var size = props.size || "default"
    // Thread the group name into item children (items resolve their context
    // key without repeating `name` on every item).
    if(props.children && props.children.map) {
        props.children = props.children.map((c) => {
            if(c && c.p && c.p.props) {
                c.p.props.__rgName = props.name || "default"
            }
            return c
        })
    }
    var current = props.value != null ? props.value : selected
    var toggle = (v) => {
        if(disabled) { return }
        var next = current
        if(multiple) {
            if(current.indexOf(v) != -1) {
                next = current.filter(x => x != v)
            } else {
                next = current.concat(v)
            }
        } else {
            next = v
        }
        // Publish directly so both modes keep items in sync immediately.
        ctx.value = next
        if(props.value != null) {
            if(props.onValueChange) { props.onValueChange(next) }
        } else {
            selected = next
            if(props.onValueChange) { props.onValueChange(next) }
        }
    }
    ctx.value = current
    ctx.mode = multiple ? "multiple" : "single"
    ctx.write = (v) => { toggle(v) }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    return <div role="group" {...props} class={classes + " " + ${toggle_group_styles(page)}} data-disabled={disabled ? "true" : "false"}>{props.children}</div>
}

public #universal ToggleGroupItem(props) {
    const ctx = useContext("tg-" + (props.__rgName || props.name || "default"))
    var disabled = props.disabled || false
    var variant = props.variant || "default"
    var size = props.size || "default"
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    // Reactive pressed state from context: single mode compares directly,
    // multiple mode checks array membership.
    var pressed = ctx.mode == "multiple" ? ctx.value.indexOf(props.value) != -1 : ctx.value == props.value
    var onToggle = () => {
        if(disabled) { return }
        if(ctx.write) { ctx.write(props.value) }
    }
    return <button {...props} type="button" aria-pressed={pressed ? "true" : "false"} data-pressed={pressed ? "true" : "false"} data-variant={variant} data-size={size} data-disabled={disabled ? "true" : "false"} disabled={disabled} class={classes + " " + ${toggle_group_item_styles(page)}} onClick={onToggle}>{props.children}</button>
}
