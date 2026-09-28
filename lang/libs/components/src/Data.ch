// Shadcn-style Progress bar with optional label/value.
// Usage:
//   <Progress value={56} />
//   <Progress value={56}><ProgressLabel>Upload progress</ProgressLabel><ProgressValue /></Progress>
//   <Progress value={56} variant="success" size="lg" />
public #universal Progress(props) {
    var progress = style {
        appearance: none;
        width: 100%;
        height: 8px;
        border-radius: 999px;
        background: hsl(var(--muted));
        border: 0;
        overflow: hidden;
        &::-webkit-progress-bar {
            border-radius: 999px;
            background: hsl(var(--muted));
        }
        &::-webkit-progress-value {
            border-radius: 999px;
            background: linear-gradient(90deg, hsl(var(--accent)), #6366f1);
            transition: width 0.3s ease;
        }
        &::-moz-progress-bar {
            border-radius: 999px;
            background: linear-gradient(90deg, hsl(var(--accent)), #6366f1);
        }
        &[data-variant="primary"]::-webkit-progress-value {
            background: hsl(var(--primary));
        }
        &[data-variant="primary"]::-moz-progress-bar {
            background: hsl(var(--primary));
        }
        &[data-variant="success"]::-webkit-progress-value {
            background: hsl(var(--success));
        }
        &[data-variant="success"]::-moz-progress-bar {
            background: hsl(var(--success));
        }
        &[data-variant="warning"]::-webkit-progress-value {
            background: hsl(var(--warning));
        }
        &[data-variant="warning"]::-moz-progress-bar {
            background: hsl(var(--warning));
        }
        &[data-variant="error"]::-webkit-progress-value {
            background: hsl(var(--destructive));
        }
        &[data-variant="error"]::-moz-progress-bar {
            background: hsl(var(--destructive));
        }
        &[data-variant="info"]::-webkit-progress-value {
            background: hsl(var(--info));
        }
        &[data-variant="info"]::-moz-progress-bar {
            background: hsl(var(--info));
        }
        &[data-size="sm"] {
            height: 4px;
        }
        &[data-size="lg"] {
            height: 12px;
        }
    }
    var progress_label = style {
        font-size: 0.8125rem;
        font-weight: 500;
        color: hsl(var(--foreground));
        margin-bottom: 0.25rem;
    }
    var progress_value = style {
        font-size: 0.8125rem;
        font-weight: 600;
        color: hsl(var(--foreground));
        margin-left: 0.5rem;
    }
    var variant = props.variant || "default"
    var size = props.size || "md"
    var value = props.value || 0
    var max = props.max || 100
    return <div style="display:grid;gap:0.25rem;width:100%;">
        {props.label !== undefined ? <span class={progress_label}>{props.label}</span> : null}
        <div style="display:flex;align-items:center;gap:0.5rem;">
            <progress {...props} max={max} value={value} data-variant={variant} data-size={size} class={progress}></progress>
            {props.showValue ? <span class={progress_value}>{value}{props.max ? "/" + max : ""}{props.suffix || ""}</span> : null}
        </div>
    </div>
}

public #universal ProgressLabel(props) {
    var progress_label = style {
        font-size: 0.8125rem;
        font-weight: 500;
        color: hsl(var(--foreground));
        margin-bottom: 0.25rem;
    }
    return <span class={progress_label}>{props.children}</span>
}

public #universal ProgressValue(props) {
    var progress_value = style {
        font-size: 0.8125rem;
        font-weight: 600;
        color: hsl(var(--foreground));
        margin-left: 0.5rem;
    }
    return <span class={progress_value}>{props.children}</span>
}


// Shadcn-style Accordion: manages open state for child AccordionItems.
// `defaultValue` is an array of item values open by default.
// `multiple` allows multiple items open at the same time.
// Accordion: styled wrapper for a group of AccordionItems.
// Passes through all props. Keyboard navigation: ArrowUp/Down move between
// triggers, Home/End jump to first/last trigger.
public #universal Accordion(props) {
    var accordion = style {
        width: 100%;
    }
    var multiple = props.multiple || false
    var handleAccordionKeyDown = (e) => {
        var all = e.currentTarget.querySelectorAll("[data-accordion-trigger]")
        if(!all || all.length == 0) { return }
        // Collect non-disabled triggers
        var triggers = []
        for(var i = 0; i < all.length; i++) {
            if(!all[i].disabled) { triggers.push(all[i]) }
        }
        if(triggers.length == 0) { return }
        var current = document.activeElement
        var idx = -1
        for(var j = 0; j < triggers.length; j++) {
            if(triggers[j] == current) { idx = j; break }
        }
        if(idx == -1) { return }
        var next = -1
        if(e.key == "ArrowDown") {
            e.preventDefault()
            next = (idx + 1) % triggers.length
        } else if(e.key == "ArrowUp") {
            e.preventDefault()
            next = (idx - 1 + triggers.length) % triggers.length
        } else if(e.key == "Home") {
            e.preventDefault()
            next = 0
        } else if(e.key == "End") {
            e.preventDefault()
            next = triggers.length - 1
        } else {
            return
        }
        triggers[next].focus()
    }
    return <div {...props} class={accordion} data-accordion-root="true" data-multiple={multiple ? "true" : "false"} onKeyDown={handleAccordionKeyDown}>
        {props.children}
    </div>
}

// AccordionItem: self-contained item with trigger + content.
// Manages its own open/close state. Accepts:
//   trigger     - label text or JSX for the clickable header
//   defaultOpen - initial open state (default false)
//   disabled    - disables toggle
//   value       - identifier for external querying
//   children    - collapsible content
public #universal AccordionItem(props) {
    var accordion_item = style {
        border-bottom: 1px solid hsl(var(--border));
    }
    var accordion_trigger = style {
        display: flex;
        align-items: center;
        justify-content: space-between;
        width: 100%;
        padding: 1rem 0;
        font-size: 0.9375rem;
        font-weight: 600;
        text-align: left;
        color: hsl(var(--foreground));
        background: transparent;
        border: 0;
        cursor: pointer;
        line-height: 1.5;
        transition: color 0.15s ease;
        &:hover {
            text-decoration: underline;
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
            border-radius: var(--radius);
        }
        &[disabled] {
            cursor: not-allowed;
            opacity: 0.5;
            pointer-events: none;
        }
    }
    var accordion_icon = style {
        width: 1rem;
        height: 1rem;
        flex-shrink: 0;
        color: hsl(var(--muted-foreground));
        transition: transform 0.2s ease;
        transform: rotate(0deg);
    }
    var accordion_content = style {
        overflow: hidden;
        font-size: 0.875rem;
        color: hsl(var(--muted-foreground));
        [data-state="open"] & {
            animation: accordion-down 0.2s ease-out;
        }
        [data-state="closed"] & {
            animation: accordion-up 0.2s ease-out;
        }
    }
    var accordion_content_inner = style {
        padding: 0 0 1rem 0;
    }
    state open = props.defaultOpen ? true : false
    var disabled = props.disabled || false
    var itemValue = props.value || ""
    var toggle = () => {
        if(disabled) { return }
        open = !open
    }
    var chevronSvg = '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"/></svg>'
    return <div {...props} class={accordion_item} data-state={open ? "open" : "closed"} data-value={itemValue} data-disabled={disabled ? "true" : "false"}>
        <button type="button" class={"chx-accordion-trigger " + accordion_trigger} onClick={toggle} disabled={disabled} aria-expanded={open ? "true" : "false"} data-accordion-trigger="true">
            <span>{props.trigger || ""}</span>
            <span class={"chx-accordion-icon " + accordion_icon} style={open ? "transform:rotate(180deg)" : ""}>
                <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"/></svg>
            </span>
        </button>
        <div class={accordion_content} style={open ? "" : "display:none;"} data-accordion-content="true" role="region">
            <div class={accordion_content_inner}>{props.children}</div>
        </div>
    </div>
}

// Standalone AccordionTrigger: styled clickable header (no state).
// Use inside AccordionItem for composition, or standalone for custom layouts.
public #universal AccordionTrigger(props) {
    var accordion_trigger = style {
        display: flex;
        align-items: center;
        justify-content: space-between;
        width: 100%;
        padding: 1rem 0;
        font-size: 0.9375rem;
        font-weight: 600;
        text-align: left;
        color: hsl(var(--foreground));
        background: transparent;
        border: 0;
        cursor: pointer;
        line-height: 1.5;
        transition: color 0.15s ease;
        &:hover {
            text-decoration: underline;
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
            border-radius: var(--radius);
        }
        &[disabled] {
            cursor: not-allowed;
            opacity: 0.5;
            pointer-events: none;
        }
    }
    var accordion_icon = style {
        width: 1rem;
        height: 1rem;
        flex-shrink: 0;
        color: hsl(var(--muted-foreground));
        transition: transform 0.2s ease;
        transform: rotate(0deg);
    }
    return <button type="button" class={"chx-accordion-trigger " + accordion_trigger} data-accordion-trigger="true">
        {props.children}
        <span class={"chx-accordion-icon " + accordion_icon}>
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"/></svg>
        </span>
    </button>
}

// Standalone AccordionContent: collapsible content panel (no state).
public #universal AccordionContent(props) {
    var accordion_content = style {
        overflow: hidden;
        font-size: 0.875rem;
        color: hsl(var(--muted-foreground));
        [data-state="open"] & {
            animation: accordion-down 0.2s ease-out;
        }
        [data-state="closed"] & {
            animation: accordion-up 0.2s ease-out;
        }
    }
    var accordion_content_inner = style {
        padding: 0 0 1rem 0;
    }
    return <div class={accordion_content} role="region">
        <div class={accordion_content_inner}>{props.children}</div>
    </div>
}

// Shadcn-style Tabs with variant/orientation support.
// Modes:
// 1. Declarative (shadcn): <Tabs defaultValue="acc"><TabList><Tab value="acc">...</Tab></TabList><TabContent value="acc">...</TabContent></Tabs>
// 2. Array mode: <Tabs tabs={[...]} panels={[...]} defaultIndex={0} />
// 3. Fallback: <Tabs><TabList>...</TabList><TabPanel>...</TabPanel></Tabs>
// Variant: "line" (default) | "pills"
// Orientation: "horizontal" (default) | "vertical"
public #universal Tabs(props) {
    var tabs = style {
        display: grid;
        gap: 1rem;
        &[data-orientation="vertical"] {
            grid-template-columns: auto 1fr;
            gap: 0;
        }
    }
    var tab_list = style {
        display: inline-flex;
        align-items: center;
        gap: 0;
        border-bottom: 1px solid hsl(var(--border));
        &[data-variant="line"] {
            border-bottom: 1px solid hsl(var(--border));
        }
        &[data-variant="pills"] {
            gap: 0.25rem;
            padding: 0.25rem;
            border: 1px solid hsl(var(--border));
            border-radius: 999px;
            background: hsl(var(--background));
            border-bottom: 1px solid hsl(var(--border));
        }
        &[data-orientation="vertical"] {
            flex-direction: column;
            border-bottom: none;
            border-right: 1px solid hsl(var(--border));
            align-items: stretch;
            &[data-variant="pills"] {
                border-right: none;
                border-bottom: none;
            }
        }
    }
    var tab = style {
        border: 0;
        padding: 0.65rem 1rem;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-weight: 500;
        font-size: 0.875rem;
        cursor: pointer;
        border-bottom: 2px solid transparent;
        margin-bottom: -1px;
        transition: color 0.15s ease, border-color 0.15s ease;
        white-space: nowrap;
        &:hover {
            color: hsl(var(--foreground));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: -2px;
            border-radius: var(--radius) var(--radius) 0 0;
        }
        &[data-variant="pills"] {
            border-bottom: none;
            margin-bottom: 0;
            border-radius: 999px;
            padding: 0.5rem 0.9rem;
        }
        &[data-orientation="vertical"] {
            border-bottom: none;
            border-right: 2px solid transparent;
            margin-bottom: 0;
            margin-right: -1px;
            text-align: left;
            justify-content: flex-start;
            &[data-variant="pills"] {
                border-right: none;
                margin-right: 0;
            }
        }
        &[data-active="true"] {
            color: hsl(var(--foreground));
            border-bottom-color: hsl(var(--primary));
            font-weight: 600;
            &[data-variant="pills"] {
                background: hsl(var(--primary));
                color: hsl(var(--primary-foreground));
                border-bottom-color: transparent;
            }
            &[data-orientation="vertical"] {
                border-bottom-color: transparent;
                border-right-color: hsl(var(--primary));
                &[data-variant="pills"] {
                    border-right-color: transparent;
                }
            }
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            cursor: not-allowed;
            pointer-events: none;
        }
    }
    var tab_panel = style {
        padding: 1rem 0;
        font-size: 0.875rem;
        line-height: 1.6;
        color: hsl(var(--foreground));
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
    }
    state active = props.defaultIndex != null ? props.defaultIndex : (props.defaultValue != null ? props.defaultValue : 0)
    var variant = props.variant || "line"
    var orientation = props.orientation || "horizontal"
    var disabled = props.disabled || false
    var baseId = props.id || "tabs"
    const tabListRef = useRef(null)

    var select = (val) => {
        active = val
        if(props.onValueChange) { props.onValueChange(val) }
        if(props.onChange) { props.onChange(val) }
    }

    // Keyboard navigation (WAI-ARIA tabs pattern)
    var handleTabKeyDown = (e) => {
        if(!tabListRef.current) { return }
        const tabs = tabListRef.current.querySelectorAll("[role=tab]")
        if(tabs.length == 0) { return }
        var currentIndex = -1
        for(var t = 0; t < tabs.length; t++) {
            if(tabs[t] == document.activeElement) { currentIndex = t; break }
        }
        if(currentIndex < 0) { currentIndex = 0 }
        var next = currentIndex
        var isVertical = orientation == "vertical"
        var nextKey = isVertical ? "ArrowDown" : "ArrowRight"
        var prevKey = isVertical ? "ArrowUp" : "ArrowLeft"
        if(e.key == nextKey) {
            e.preventDefault()
            next = (currentIndex + 1) % tabs.length
        } else if(e.key == prevKey) {
            e.preventDefault()
            next = (currentIndex - 1 + tabs.length) % tabs.length
        } else if(e.key == "Home") {
            e.preventDefault()
            next = 0
        } else if(e.key == "End") {
            e.preventDefault()
            next = tabs.length - 1
        } else {
            return
        }
        tabs[next].focus()
        // Extract value from data-value attribute
        var tabValue = tabs[next].getAttribute("data-tab-value")
        if(tabValue != null) {
            select(tabValue)
        } else {
            select(next)
        }
    }

    // Array mode
    if(props.tabs) {
        return <div {...props} class={tabs} data-orientation={orientation}>
            <div ref={tabListRef} class={tab_list} data-variant={variant} data-orientation={orientation} role="tablist" aria-label={props.ariaLabel} aria-orientation={orientation} onKeyDown={handleTabKeyDown}>
                {props.tabs.map((tab, i) => (
                    <button type="button" onClick={() => select(i)} data-tab-value={i} tabIndex={active == i ? 0 : -1} id={baseId + "-tab-" + i} aria-controls={baseId + "-panel-" + i} data-variant={variant} data-orientation={orientation} data-disabled={disabled ? "true" : "false"} data-active={active == i ? "true" : "false"} class={tab} role="tab" aria-selected={active == i ? "true" : "false"}>{tab}</button>
                ))}
            </div>
            <div>
                {props.panels.map((panel, i) => (
                    <div role="tabpanel" id={baseId + "-panel-" + i} aria-labelledby={baseId + "-tab-" + i} class={tab_panel} style={active == i ? "" : "display:none;"} tabindex={0}>{panel}</div>
                ))}
            </div>
        </div>
    }

    // Fallback: plain wrapper
    return <div {...props} class={tabs} data-orientation={orientation}>{props.children}</div>
}

public #universal TabList(props) {
    var tab_list = style {
        display: inline-flex;
        align-items: center;
        gap: 0;
        border-bottom: 1px solid hsl(var(--border));
        &[data-variant="line"] {
            border-bottom: 1px solid hsl(var(--border));
        }
        &[data-variant="pills"] {
            gap: 0.25rem;
            padding: 0.25rem;
            border: 1px solid hsl(var(--border));
            border-radius: 999px;
            background: hsl(var(--background));
            border-bottom: 1px solid hsl(var(--border));
        }
        &[data-orientation="vertical"] {
            flex-direction: column;
            border-bottom: none;
            border-right: 1px solid hsl(var(--border));
            align-items: stretch;
            &[data-variant="pills"] {
                border-right: none;
                border-bottom: none;
            }
        }
    }
    var variant = props.variant || "line"
    var orientation = props.orientation || "horizontal"
    return <div {...props} data-variant={variant} data-orientation={orientation} class={tab_list} role="tablist" aria-orientation={orientation}>{props.children}</div>
}

public #universal Tab(props) {
    var tab = style {
        border: 0;
        padding: 0.65rem 1rem;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-weight: 500;
        font-size: 0.875rem;
        cursor: pointer;
        border-bottom: 2px solid transparent;
        margin-bottom: -1px;
        transition: color 0.15s ease, border-color 0.15s ease;
        white-space: nowrap;
        &:hover {
            color: hsl(var(--foreground));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: -2px;
            border-radius: var(--radius) var(--radius) 0 0;
        }
        &[data-variant="pills"] {
            border-bottom: none;
            margin-bottom: 0;
            border-radius: 999px;
            padding: 0.5rem 0.9rem;
        }
        &[data-orientation="vertical"] {
            border-bottom: none;
            border-right: 2px solid transparent;
            margin-bottom: 0;
            margin-right: -1px;
            text-align: left;
            justify-content: flex-start;
            &[data-variant="pills"] {
                border-right: none;
                margin-right: 0;
            }
        }
        &[data-active="true"] {
            color: hsl(var(--foreground));
            border-bottom-color: hsl(var(--primary));
            font-weight: 600;
            &[data-variant="pills"] {
                background: hsl(var(--primary));
                color: hsl(var(--primary-foreground));
                border-bottom-color: transparent;
            }
            &[data-orientation="vertical"] {
                border-bottom-color: transparent;
                border-right-color: hsl(var(--primary));
                &[data-variant="pills"] {
                    border-right-color: transparent;
                }
            }
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            cursor: not-allowed;
            pointer-events: none;
        }
    }
    var disabled = props.disabled || false
    return <button {...props} type="button" data-disabled={disabled ? "true" : "false"} data-active={props.selected ? "true" : "false"} data-variant={props.variant} data-orientation={props.orientation} class={tab} role="tab" aria-selected={props.selected ? "true" : "false"} tabIndex={props.selected ? 0 : -1} disabled={disabled}>{props.children}</button>
}

public #universal TabActive(props) {
    var tab = style {
        border: 0;
        padding: 0.65rem 1rem;
        background: transparent;
        color: hsl(var(--muted-foreground));
        font-weight: 500;
        font-size: 0.875rem;
        cursor: pointer;
        border-bottom: 2px solid transparent;
        margin-bottom: -1px;
        transition: color 0.15s ease, border-color 0.15s ease;
        white-space: nowrap;
        &:hover {
            color: hsl(var(--foreground));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: -2px;
            border-radius: var(--radius) var(--radius) 0 0;
        }
        &[data-variant="pills"] {
            border-bottom: none;
            margin-bottom: 0;
            border-radius: 999px;
            padding: 0.5rem 0.9rem;
        }
        &[data-orientation="vertical"] {
            border-bottom: none;
            border-right: 2px solid transparent;
            margin-bottom: 0;
            margin-right: -1px;
            text-align: left;
            justify-content: flex-start;
            &[data-variant="pills"] {
                border-right: none;
                margin-right: 0;
            }
        }
        &[data-active="true"] {
            color: hsl(var(--foreground));
            border-bottom-color: hsl(var(--primary));
            font-weight: 600;
            &[data-variant="pills"] {
                background: hsl(var(--primary));
                color: hsl(var(--primary-foreground));
                border-bottom-color: transparent;
            }
            &[data-orientation="vertical"] {
                border-bottom-color: transparent;
                border-right-color: hsl(var(--primary));
                &[data-variant="pills"] {
                    border-right-color: transparent;
                }
            }
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            cursor: not-allowed;
            pointer-events: none;
        }
    }
    return <button {...props} type="button" data-active="true" data-variant={props.variant} data-orientation={props.orientation} class={tab} role="tab" aria-selected="true" tabIndex={0}>{props.children}</button>
}

public #universal TabPanel(props) {
    var tab_panel = style {
        padding: 1rem 0;
        font-size: 0.875rem;
        line-height: 1.6;
        color: hsl(var(--foreground));
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
    }
    return <div {...props} class={tab_panel} role="tabpanel" tabindex={0}>{props.children}</div>
}

// Pagination in two modes:
// - `pages` array + optional `defaultPage`: the component owns the current
//   page state, renders prev/next controls and the page buttons. The boundary
//   arrows disable at the first/last page. `prevLabel`/`nextLabel` (default
//   "‹"/"›") override the arrows; `onChange` fires with the new page.
// - Otherwise it degrades to the plain styled wrapper for explicit
//   <PageItem>/<PageItemActive> children.
public #universal Pagination(props) {
    var pagination = style {
        display: inline-flex;
        align-items: center;
        gap: 0.55rem;
        flex-wrap: wrap;
    }
    var pagination_item = style {
        min-width: 2.4rem;
        height: 2.4rem;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 0 0.85rem;
        border-radius: 999px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        text-decoration: none;
        font-weight: 600;
    }
    state current = props.defaultPage ? props.defaultPage : 1
    if(props.pages) {
        var prevLabel = props.prevLabel || "‹"
        var nextLabel = props.nextLabel || "›"
        var goPrev = () => {
            if(current > 1) {
                current = current - 1
                if(props.onChange) { props.onChange(current) }
            }
        }
        var goNext = () => {
            if(current < props.pages.length) {
                current = current + 1
                if(props.onChange) { props.onChange(current) }
            }
        }
        var goTo = (p) => {
            current = p
            if(props.onChange) { props.onChange(p) }
        }
        return <nav {...props} class={pagination}>
            <button type="button" onClick={goPrev} disabled={current <= 1} class={pagination_item} aria-label="Previous page">{prevLabel}</button>
            {props.pages.map(p => (
                <button type="button" onClick={() => goTo(p)} class={pagination_item} style={current == p ? "background:hsl(var(--primary));color:hsl(var(--primary-foreground));border-color:transparent;" : ""} aria-current={current == p ? "page" : null}>{p}</button>
            ))}
            <button type="button" onClick={goNext} disabled={current >= props.pages.length} class={pagination_item} aria-label="Next page">{nextLabel}</button>
        </nav>
    }
    return <nav {...props} class={pagination}>{props.children}</nav>
}

public #universal PageItem(props) {
    var pagination_item = style {
        min-width: 2.4rem;
        height: 2.4rem;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        padding: 0 0.85rem;
        border-radius: 999px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        text-decoration: none;
        font-weight: 600;
    }
    return <a {...props} class={pagination_item}>{props.children}</a>
}

public #universal PageItemActive(props) {
    var pagination_active = style {
        background: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
        border-color: transparent;
    }
    return <PageItem {...props} class={pagination_active}>{props.children}</PageItem>
}

public #universal List(props) {
    var list = style {
        margin: 0;
        padding: 0;
        list-style: none;
        display: grid;
        gap: 0.55rem;
    }
    return <ul {...props} class={list}>{props.children}</ul>
}

public #universal ListItem(props) {
    var list_item = style {
        padding: 0.85rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 12px;
        background: hsl(var(--background));
        color: hsl(var(--foreground));
    }
    return <li {...props} class={list_item}>{props.children}</li>
}

public #universal Table(props) {
    var table = style {
        width: 100%;
        border-collapse: separate;
        border-spacing: 0;
        overflow: hidden;
        border-radius: 14px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
    }
    return <table {...props} class={table}>{props.children}</table>
}

public #universal TableHeadCell(props) {
    var table_head_cell = style {
        padding: 0.85rem 1rem;
        text-align: left;
        background: hsl(var(--muted));
        color: hsl(var(--foreground));
        font-size: 0.82rem;
        text-transform: uppercase;
        letter-spacing: 0.05em;
        border-bottom: 1px solid hsl(var(--border));
    }
    return <th {...props} class={table_head_cell}>{props.children}</th>
}

public #universal TableCell(props) {
    var table_cell = style {
        padding: 0.9rem 1rem;
        color: hsl(var(--foreground));
        border-bottom: 1px solid hsl(var(--border));
    }
    return <td {...props} class={table_cell}>{props.children}</td>
}
