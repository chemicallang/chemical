func progress_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

func accordion_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
    }
}

func accordion_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        border-bottom: 1px solid hsl(var(--border));
    }
}

func accordion_trigger_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

func accordion_icon_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 1rem;
        height: 1rem;
        flex-shrink: 0;
        color: hsl(var(--muted-foreground));
        transition: transform 0.2s ease;
        transform: rotate(0deg);
    }
}

func accordion_content_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

func accordion_content_inner_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0 0 1rem 0;
    }
}

func accordion_summary_styles(page : &mut HtmlPage) : *char {
    return accordion_trigger_styles(page)
}

func accordion_panel_styles(page : &mut HtmlPage) : *char {
    return accordion_content_styles(page)
}

func tabs_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.85rem;
    }
}

func tab_list_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        flex-wrap: wrap;
        gap: 0.6rem;
        padding: 0.45rem;
        border: 1px solid hsl(var(--border));
        border-radius: 999px;
        background: hsl(var(--background));
    }
}

func tab_styles(page : &mut HtmlPage) : *char {
    return #css {
        border: 0;
        padding: 0.65rem 0.95rem;
        border-radius: 999px;
        background: hsl(var(--muted));
        color: hsl(var(--foreground));
        font-weight: 600;
        cursor: pointer;
        &:hover {
            background: rgba(59, 130, 246, 0.12);
        }
    }
}

func tab_active_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
    }
}

func tab_panel_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1rem 1.1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        color: hsl(var(--foreground));
    }
}

func pagination_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.55rem;
        flex-wrap: wrap;
    }
}

func pagination_item_styles(page : &mut HtmlPage) : *char {
    return #css {
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
}

func pagination_active_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
        border-color: transparent;
    }
}

func list_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin: 0;
        padding: 0;
        list-style: none;
        display: grid;
        gap: 0.55rem;
    }
}

func list_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.85rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 12px;
        background: hsl(var(--background));
        color: hsl(var(--foreground));
    }
}

func table_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        border-collapse: separate;
        border-spacing: 0;
        overflow: hidden;
        border-radius: 14px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
    }
}

func table_head_cell_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.85rem 1rem;
        text-align: left;
        background: hsl(var(--muted));
        color: hsl(var(--foreground));
        font-size: 0.82rem;
        text-transform: uppercase;
        letter-spacing: 0.05em;
        border-bottom: 1px solid hsl(var(--border));
    }
}

func table_cell_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.9rem 1rem;
        color: hsl(var(--foreground));
        border-bottom: 1px solid hsl(var(--border));
    }
}

// Progress bar: `value` (default 0) / `max` (default 100) drive the fill;
// `variant` picks the tone (default/accent gradient, primary, success, warning,
// error, info); `size` (sm/md/lg) scales the thickness. Other props pass
// through via the spread (aria, id, className, ...).
public #universal Progress(props) {
    var variant = props.variant || "default"
    var size = props.size || "md"
    var value = props.value || 0
    var max = props.max || 100
    return <progress
        {...props}
        max={max}
        value={value}
        data-variant={variant}
        data-size={size}
        class={${progress_styles(page)}}
    ></progress>
}


// Shadcn-style Accordion: manages open state for child AccordionItems.
// `defaultValue` is an array of item values open by default.
// `multiple` allows multiple items open at the same time.
public #universal Accordion(props) {
    var multiple = props.multiple || false
    var defaultValues = props.defaultValue || []
    state openItems = defaultValues
    var toggleItem = (value) => {
        var idx = openItems.indexOf(value)
        if(idx >= 0) {
            openItems.splice(idx, 1)
        } else {
            if(!multiple) { openItems = [] }
            openItems.push(value)
        }
        if(props.onValueChange) { props.onValueChange(openItems) }
    }
    var isItemOpen = (value) => { return openItems.indexOf(value) >= 0 }
    // Keyboard navigation: ArrowDown/Up/Home/End cycle through triggers
    var handleKeyDown = (e) => {
        if(e.key != "ArrowDown" && e.key != "ArrowUp" && e.key != "Home" && e.key != "End") { return }
        const root = e.currentTarget.closest("[data-accordion-root]")
        const triggers = root ? root.querySelectorAll("button.chx-accordion-trigger:not([disabled])") : []
        if(triggers.length == 0) { return }
        var idx = -1
        for(var t = 0; t < triggers.length; t++) {
            if(triggers[t] == e.currentTarget) { idx = t; break }
        }
        if(idx < 0) { return }
        var next = idx
        if(e.key == "ArrowDown") { e.preventDefault(); next = (idx + 1) % triggers.length }
        else if(e.key == "ArrowUp") { e.preventDefault(); next = (idx - 1 + triggers.length) % triggers.length }
        else if(e.key == "Home") { e.preventDefault(); next = 0 }
        else if(e.key == "End") { e.preventDefault(); next = triggers.length - 1 }
        triggers[next].focus()
    }
    return <div {...props} class={${accordion_styles(page)}} data-accordion-root="true" data-state="open" data-multiple={multiple ? "true" : "false"} onKeyDown={handleKeyDown}>
        {props.children}
    </div>
}

// Shadcn-style AccordionItem: wraps a trigger + content pair.
public #universal AccordionItem(props) {
    var itemValue = props.value || ""
    var disabled = props.disabled || false
    return <div {...props} class={${accordion_item_styles(page)}} data-state="open" data-disabled={disabled ? "true" : "false"} data-value={itemValue}>
        {props.children}
    </div>
}

// Shadcn-style AccordionTrigger: the clickable header.
public #universal AccordionTrigger(props) {
    return <button type="button" class={"chx-accordion-trigger " + ${accordion_trigger_styles(page)}} data-state="open" aria-expanded="true">
        {props.children}
        <span class={${accordion_icon_styles(page)}}>
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m6 9 6 6 6-6"/></svg>
        </span>
    </button>
}

// Shadcn-style AccordionContent: the collapsible content panel.
public #universal AccordionContent(props) {
    return <div class={${accordion_content_styles(page)}} data-state="open" role="region">
        <div class={${accordion_content_inner_styles(page)}}>{props.children}</div>
    </div>
}

// Legacy aliases
public #universal AccordionSummary(props) {
    return <AccordionTrigger {...props}>{props.children}</AccordionTrigger>
}
public #universal AccordionPanel(props) {
    return <AccordionContent {...props}>{props.children}</AccordionContent>
}

// Legacy: old-style accordion item with title/subtitle props.
public #universal AccordionItemLegacy(props) {
    state open = props.defaultOpen ? true : false
    var disabled = props.disabled || false
    var toggle = () => {
        if(disabled) { return }
        open = !open
    }
    return <div {...props} class={${accordion_item_styles(page)}} data-disabled={disabled ? "true" : "false"}>
        <button type="button" class="chx-accordion-summary" onClick={toggle} disabled={disabled} aria-expanded={open ? "true" : "false"}>
            <span class="chx-accordion-copy">
                <span class="chx-accordion-title">{props.title}</span>
                <span class="chx-accordion-subtitle">{props.subtitle}</span>
            </span>
            <span class="chx-accordion-icon">{open ? "+" : "+"}</span>
        </button>
        <div class="chx-accordion-panel" style={open ? "" : "display:none;"}>{props.children}</div>
    </div>
}

// Tabs in two modes:
// - `tabs`/`panels` arrays + optional `defaultIndex`: the component owns the
//   active-tab state, renders the tab buttons and toggles panel visibility.
// - Otherwise it degrades to the plain styled wrapper so explicit
//   <TabList>/<Tab>/<TabPanel> children keep working unchanged.
public #universal Tabs(props) {
    state active = props.defaultIndex ? props.defaultIndex : 0
    if(props.tabs) {
        const tabListRef = useRef(null)
        var select = (i) => {
            active = i
            if(props.onChange) { props.onChange(i) }
        }
        // Keyboard navigation (shadcn/WAI-ARIA tabs pattern): Left/Right move
        // focus + selection between tabs (wrapping), Home/End jump to the
        // first/last. The active tab is the only one in the tab order
        // (roving tabindex).
        var handleTabKeyDown = (e) => {
            if(!tabListRef.current) {
                return
            }
            const tabs = tabListRef.current.querySelectorAll("[role=tab]")
            if(tabs.length == 0) {
                return
            }
            var currentIndex = -1
            for(var t = 0; t < tabs.length; t++) {
                if(tabs[t] == document.activeElement) {
                    currentIndex = t
                    break
                }
            }
            if(currentIndex < 0) { currentIndex = active }
            var next = currentIndex
            if(e.key == "ArrowRight") {
                e.preventDefault()
                next = (currentIndex + 1) % tabs.length
            } else if(e.key == "ArrowLeft") {
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
            select(next)
        }
        // ARIA wiring: each tab controls its panel (aria-controls), each panel
        // is labelled by its tab (aria-labelledby), giving tabpanels an
        // accessible name derived from the tab text.
        var baseId = props.id ? props.id : "tabs"
        return <div {...props} class={${tabs_styles(page)}}>
            <div ref={tabListRef} class={${tab_list_styles(page)}} role="tablist" aria-label={props.ariaLabel} onKeyDown={handleTabKeyDown}>
                {props.tabs.map((tab, i) => (
                    <button type="button" onClick={() => select(i)} tabIndex={active == i ? 0 : -1} id={baseId + "-tab-" + i} aria-controls={baseId + "-panel-" + i} class={${tab_styles(page)}} style={active == i ? "background:hsl(var(--primary));color:hsl(var(--primary-foreground));border-color:transparent;" : ""} role="tab" aria-selected={active == i ? "true" : "false"}>{tab}</button>
                ))}
            </div>
            <div class="chx-tabs-content" style="display:grid;gap:0.85rem;">
                {props.panels.map((panel, i) => (
                    <div role="tabpanel" id={baseId + "-panel-" + i} aria-labelledby={baseId + "-tab-" + i} style={active == i ? "" : "display:none;"}>{panel}</div>
                ))}
            </div>
        </div>
    }
    return <div {...props} class={${tabs_styles(page)}}>{props.children}</div>
}

public #universal TabList(props) {
    return <div {...props} class={${tab_list_styles(page)}}>{props.children}</div>
}

public #universal Tab(props) {
    return <button {...props} type="button" class={${tab_styles(page)}}>{props.children}</button>
}

public #universal TabActive(props) {
    return <Tab {...props} class={${tab_active_styles(page)}}>{props.children}</Tab>
}

public #universal TabPanel(props) {
    return <div {...props} class={${tab_panel_styles(page)}}>{props.children}</div>
}

// Pagination in two modes:
// - `pages` array + optional `defaultPage`: the component owns the current
//   page state, renders prev/next controls and the page buttons. The boundary
//   arrows disable at the first/last page. `prevLabel`/`nextLabel` (default
//   "‹"/"›") override the arrows; `onChange` fires with the new page.
// - Otherwise it degrades to the plain styled wrapper for explicit
//   <PageItem>/<PageItemActive> children.
public #universal Pagination(props) {
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
        return <nav {...props} class={${pagination_styles(page)}}>
            <button type="button" onClick={goPrev} disabled={current <= 1} class={${pagination_item_styles(page)}} aria-label="Previous page">{prevLabel}</button>
            {props.pages.map(p => (
                <button type="button" onClick={() => goTo(p)} class={${pagination_item_styles(page)}} style={current == p ? "background:hsl(var(--primary));color:hsl(var(--primary-foreground));border-color:transparent;" : ""} aria-current={current == p ? "page" : null}>{p}</button>
            ))}
            <button type="button" onClick={goNext} disabled={current >= props.pages.length} class={${pagination_item_styles(page)}} aria-label="Next page">{nextLabel}</button>
        </nav>
    }
    return <nav {...props} class={${pagination_styles(page)}}>{props.children}</nav>
}

public #universal PageItem(props) {
    return <a {...props} class={${pagination_item_styles(page)}}>{props.children}</a>
}

public #universal PageItemActive(props) {
    return <PageItem {...props} class={${pagination_active_styles(page)}}>{props.children}</PageItem>
}

public #universal List(props) {
    return <ul {...props} class={${list_styles(page)}}>{props.children}</ul>
}

public #universal ListItem(props) {
    return <li {...props} class={${list_item_styles(page)}}>{props.children}</li>
}

public #universal Table(props) {
    return <table {...props} class={${table_styles(page)}}>{props.children}</table>
}

public #universal TableHeadCell(props) {
    return <th {...props} class={${table_head_cell_styles(page)}}>{props.children}</th>
}

public #universal TableCell(props) {
    return <td {...props} class={${table_cell_styles(page)}}>{props.children}</td>
}
