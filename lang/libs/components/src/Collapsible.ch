// Shadcn-style Collapsible: a stateful show/hide region.
//
// Props:
//   open              controlled open state (bool)
//   defaultOpen       uncontrolled initial state (SSR uses this)
//   onOpenChange      fired with the next state when toggled
//   trigger           required — the always-visible control (text or JSX)
//   disabled          locks the trigger
//   className         merged with the root style class
//   children          the collapsible content (hidden when closed)
//
// Works in both controlled (`open` set) and uncontrolled modes, matching
// the Dialog/Dropdown conventions used elsewhere in the library.

func collapsible_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0;
        width: 100%;
    }
}

func collapsible_trigger_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.5rem;
        width: 100%;
        padding: 0.75rem 0;
        border: none;
        background: transparent;
        color: hsl(var(--foreground));
        font-size: 0.875rem;
        font-weight: 600;
        text-align: left;
        cursor: pointer;
        user-select: none;
        &:hover {
            color: hsl(var(--primary));
        }
        &:disabled {
            opacity: 0.5;
            cursor: not-allowed;
            &:hover { color: hsl(var(--foreground)); }
        }
        .chx-collapsible-chevron {
            transition: transform 0.2s var(--ease);
            font-size: 0.75rem;
            color: hsl(var(--muted-foreground));
            flex-shrink: 0;
        }
        &[data-open="true"] .chx-collapsible-chevron {
            transform: rotate(180deg);
        }
    }
}

func collapsible_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        overflow: hidden;
        transition: height 0.25s var(--ease), opacity 0.2s ease;
        color: hsl(var(--muted-foreground));
        font-size: 0.875rem;
        line-height: 1.6;
    }
}

public #universal Collapsible(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    var disabled = props.disabled || false
    var toggle = () => {
        if(disabled) {
            return
        }
        if(props.open != null) {
            if(props.onOpenChange) {
                props.onOpenChange(!isOpen)
            }
        } else {
            open = !open
            if(props.onOpenChange) {
                props.onOpenChange(open)
            }
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var triggerClasses = props.triggerClassName || ""
    return <div class={classes + " " + ${collapsible_styles(page)}}>
        <button type="button" data-open={isOpen ? "true" : "false"} disabled={disabled} onClick={toggle} aria-expanded={isOpen ? "true" : "false"} class={triggerClasses + " " + ${collapsible_trigger_styles(page)}}>
            <span>{props.trigger}</span>
            <span class="chx-collapsible-chevron">▾</span>
        </button>
        <div class={${collapsible_content_styles(page)}} style={isOpen ? "" : "display:none;"}>
            {props.children}
        </div>
    </div>
}

// Low-level styled trigger/content for manual controlled composition.
public #universal CollapsibleTrigger(props) {
    return <button {...props} type="button" class={${collapsible_trigger_styles(page)}}>{props.children}</button>
}

public #universal CollapsibleContent(props) {
    return <div {...props} class={${collapsible_content_styles(page)}}>{props.children}</div>
}
