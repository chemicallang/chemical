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
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        overflow: hidden;
    }
}

func accordion_summary_styles(page : &mut HtmlPage) : *char {
    return #css {
        cursor: pointer;
        list-style: none;
        padding: 1rem 1.1rem;
        font-weight: 650;
        color: hsl(var(--foreground));
        background: hsl(var(--background));
        border-bottom: 1px solid hsl(var(--border));
    }
}

func accordion_panel_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1rem 1.1rem;
        color: hsl(var(--muted-foreground));
        background: hsl(var(--background));
    }
}

func accordion_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        border: 1px solid hsl(var(--border));
        border-radius: 18px;
        background: hsl(var(--background));
        overflow: hidden;
        box-shadow: var(--shadow-sm);
        .chx-accordion-summary {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 1rem;
            cursor: pointer;
            list-style: none;
            width: 100%;
            padding: 1rem 1.1rem;
            font-weight: 650;
            color: hsl(var(--foreground));
            background: linear-gradient(180deg, rgba(255, 255, 255, 0.02), transparent 80%), hsl(var(--background));
            border: 0;
        }
        .chx-accordion-summary::-webkit-details-marker {
            display: none;
        }
        .chx-accordion-copy {
            display: grid;
            gap: 0.2rem;
        }
        .chx-accordion-title {
            font-size: 1rem;
            color: hsl(var(--foreground));
        }
        .chx-accordion-subtitle {
            font-size: 0.84rem;
            font-weight: 500;
            color: hsl(var(--muted-foreground));
        }
        .chx-accordion-icon {
            width: 2rem;
            height: 2rem;
            border-radius: 999px;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            background: hsl(var(--muted));
            border: 1px solid hsl(var(--border));
            font-size: 1rem;
            line-height: 1;
            font-family: ui-monospace, "SFMono-Regular", monospace;
            flex-shrink: 0;
            transition: transform 0.18s ease, background 0.18s ease, color 0.18s ease, border-color 0.18s ease;
        }
        .chx-accordion-panel {
            padding: 1rem 1.1rem;
            color: hsl(var(--muted-foreground));
            background: hsl(var(--background));
            border-top: 1px solid hsl(var(--border));
        }
        &[open] .chx-accordion-icon {
            transform: rotate(45deg);
            background: hsl(var(--primary));
            color: hsl(var(--primary-foreground));
            border-color: transparent;
        }
    }
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
        border-collapse: collapse;
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

public #universal Accordion(props) {
    return <details {...props} class={${accordion_styles(page)}}>{props.children}</details>
}

public #universal AccordionSummary(props) {
    return <summary {...props} class={${accordion_summary_styles(page)}}>{props.children}</summary>
}

public #universal AccordionPanel(props) {
    return <div {...props} class={${accordion_panel_styles(page)}}>{props.children}</div>
}

// Stateful accordion item: owns its open state, swaps the chevron glyph, and
// shows/hides the panel without relying on native <details> behavior.
// `defaultOpen` controls the initial (and SSR) state; `chevronOpen`/
// `chevronClosed` override the glyphs; `disabled` locks the item; `onToggle`
// fires with the next state on every click.
public #universal AccordionItem(props) {
    state open = props.defaultOpen ? true : false
    var disabled = props.disabled || false
    var chevronOpen = props.chevronOpen || "−"
    var chevronClosed = props.chevronClosed || "+"
    var toggle = () => {
        if(disabled) {
            return
        }
        open = !open
        if(props.onToggle) {
            props.onToggle(open)
        }
    }
    return <div {...props} class={${accordion_item_styles(page)}} data-disabled={disabled ? "true" : "false"}>
        <button type="button" class="chx-accordion-summary" onClick={toggle} disabled={disabled} aria-expanded={open ? "true" : "false"}>
            <span class="chx-accordion-copy">
                <span class="chx-accordion-title">{props.title}</span>
                <span class="chx-accordion-subtitle">{props.subtitle}</span>
            </span>
            <span class="chx-accordion-icon">{open ? chevronOpen : chevronClosed}</span>
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
        var select = (i) => {
            active = i
            if(props.onChange) { props.onChange(i) }
        }
        return <div {...props} class={${tabs_styles(page)}}>
            <div class={${tab_list_styles(page)}} role="tablist" aria-label={props.ariaLabel}>
                {props.tabs.map((tab, i) => (
                    <button type="button" onClick={() => select(i)} class={${tab_styles(page)}} style={active == i ? "background:hsl(var(--primary));color:hsl(var(--primary-foreground));border-color:transparent;" : ""} role="tab" aria-selected={active == i ? "true" : "false"}>{tab}</button>
                ))}
            </div>
            <div class="chx-tabs-content" style="display:grid;gap:0.85rem;">
                {props.panels.map((panel, i) => (
                    <div role="tabpanel" style={active == i ? "" : "display:none;"}>{panel}</div>
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
