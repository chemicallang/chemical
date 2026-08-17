// Shadcn-style Select: a custom dropdown listbox (no native <select>).
//
// Two modes:
//
// 1. Options mode (stateful):
//      <Select options={["a", "b"]} defaultValue="a" onValueChange={...} placeholder="Pick one" />
//    The group owns selection + open state; `value` switches to controlled
//    mode, `onValueChange` fires with the selected string.
//
// 2. Children mode: wrap explicit <SelectItem value="...">Label</SelectItem>
//    children (or plain <option> elements). Selection is read from the clicked
//    item's `data-select-value` attribute, so custom labels work.
//
// The trigger button shows the current value (or `placeholder`); the listbox
// opens below, closes on outside click or Escape. `disabled` locks it,
// `size` mirrors the Input sizes, and `className` merges with the trigger.

func select_trigger_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.5rem;
        width: 100%;
        height: 2.5rem;
        padding: 0 0.9rem;
        border-radius: calc(var(--radius) - 2px);
        border: 1px solid hsl(var(--input));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        font-size: 0.875rem;
        text-align: left;
        cursor: pointer;
        user-select: none;
        transition: border-color 0.15s ease, box-shadow 0.15s ease;
        &:hover {
            border-color: hsl(var(--input) / 0.8);
        }
        &:focus-visible,
        &[data-open="true"] {
            border-color: hsl(var(--ring));
            box-shadow: 0 0 0 3px hsl(var(--ring) / 0.22);
            outline: none;
        }
        &:disabled {
            opacity: 0.55;
            cursor: not-allowed;
        }
        &[data-size="sm"] {
            height: 2.25rem;
            padding: 0 0.75rem;
            font-size: 0.8125rem;
        }
        &[data-size="lg"] {
            height: 2.75rem;
            padding: 0 1rem;
            font-size: 1rem;
        }
        .chx-select-value {
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
            flex: 1;
            min-width: 0;
        }
        .chx-select-placeholder {
            color: hsl(var(--muted-foreground));
        }
        .chx-select-chevron {
            color: hsl(var(--muted-foreground));
            font-size: 0.65rem;
            flex-shrink: 0;
            transition: transform 0.2s var(--ease);
        }
        &[data-open="true"] .chx-select-chevron {
            transform: rotate(180deg);
        }
    }
}

func select_menu_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        top: calc(100% + 0.375rem);
        left: 0;
        right: 0;
        min-width: 100%;
        z-index: 20;
        display: grid;
        gap: 0.125rem;
        padding: 0.375rem;
        border-radius: var(--radius-md);
        border: 1px solid hsl(var(--border));
        background: hsl(var(--popover));
        color: hsl(var(--popover-foreground));
        box-shadow: var(--shadow-lg);
        max-height: 16rem;
        overflow-y: auto;
        animation: chx-slide-down 0.15s var(--ease);
    }
}

func select_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        gap: 0.5rem;
        width: 100%;
        padding: 0.5rem 0.65rem;
        border-radius: calc(var(--radius) - 4px);
        border: none;
        background: transparent;
        color: hsl(var(--popover-foreground));
        font-size: 0.875rem;
        text-align: left;
        cursor: pointer;
        user-select: none;
        transition: background-color 0.12s ease, color 0.12s ease;
        &:hover {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &[data-selected="true"] {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
            font-weight: 600;
        }
    }
}

public #universal Select(props) {
    state open = false
    state selected = props.defaultValue || ""
    var current = props.value != null ? props.value : selected
    var disabled = props.disabled || false
    var size = props.size || "default"
    var select = (v) => {
        if(props.value != null) {
            if(props.onValueChange) { props.onValueChange(v) }
            if(props.onChange) { props.onChange(v) }
        } else {
            selected = v
            if(props.onValueChange) { props.onValueChange(v) }
            if(props.onChange) { props.onChange(v) }
        }
        open = false
    }
    var toggle = () => {
        if(disabled) {
            return
        }
        open = !open
    }
    var close = () => {
        open = false
    }
    useEffect(() => {
        const handler = (e) => {
            if(e.key === "Escape") {
                close()
            }
        }
        document.addEventListener("keydown", handler)
        return () => document.removeEventListener("keydown", handler)
    }, [])
    // Delegated selection: read the clicked item's data-select-value.
    var handleMenuClick = (e) => {
        var item = e.target.closest("[data-select-value]")
        if(item) {
            select(item.getAttribute("data-select-value"))
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var placeholder = props.placeholder || "Select..."
    return <div class={classes} style="position:relative;display:inline-block;width:100%;">
        <div onClick={close} style={open ? "position:fixed;inset:0;z-index:10;" : "display:none;"}></div>
        <button type="button" disabled={disabled} onClick={toggle} data-open={open ? "true" : "false"} data-size={size} class={${select_trigger_styles(page)}} aria-haspopup="listbox" aria-expanded={open ? "true" : "false"} aria-label={props.ariaLabel}>
            <span class={current != "" ? "chx-select-value" : "chx-select-value chx-select-placeholder"}>{current ? current : placeholder}</span>
            <span class="chx-select-chevron">▾</span>
        </button>
        <div class={${select_menu_styles(page)}} style={open ? "display:grid;" : "display:none;"} role="listbox" onClick={handleMenuClick}>
            {props.options ? props.options.map((opt, i) => (
                <button type="button" role="option" aria-selected={current == opt ? "true" : "false"} data-select-value={opt} data-selected={current == opt ? "true" : "false"} class={${select_item_styles(page)}}>{opt}</button>
            )) : props.children}
        </div>
    </div>
}

public #universal SelectItem(props) {
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var selected = props.selected || false
    return <button type="button" role="option" aria-selected={selected ? "true" : "false"} data-select-value={props.value} data-selected={selected ? "true" : "false"} class={classes + " " + ${select_item_styles(page)}}>{props.children}</button>
}
