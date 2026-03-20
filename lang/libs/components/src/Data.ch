func progress_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        height: 12px;
        accent-color: var(--chx-accent);
        border-radius: 999px;
        background: var(--chx-surface-2);
    }
}

func accordion_styles(page : &mut HtmlPage) : *char {
    return #css {
        border: 1px solid var(--chx-border);
        border-radius: 14px;
        background: var(--chx-surface);
        overflow: hidden;
    }
}

func accordion_summary_styles(page : &mut HtmlPage) : *char {
    return #css {
        cursor: pointer;
        list-style: none;
        padding: 1rem 1.1rem;
        font-weight: 650;
        color: var(--chx-text-main);
        background: var(--chx-surface);
        border-bottom: 1px solid var(--chx-border);
    }
}

func accordion_panel_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1rem 1.1rem;
        color: var(--chx-text-muted);
        background: var(--chx-surface);
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
        border: 1px solid var(--chx-border);
        border-radius: 999px;
        background: var(--chx-surface);
    }
}

func tab_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.65rem 0.95rem;
        border-radius: 999px;
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-weight: 600;
    }
}

func tab_active_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-primary);
        color: var(--chx-primary-fg);
    }
}

func tab_panel_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 1rem 1.1rem;
        border: 1px solid var(--chx-border);
        border-radius: 14px;
        background: var(--chx-surface);
        color: var(--chx-text-main);
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
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
        color: var(--chx-text-main);
        text-decoration: none;
        font-weight: 600;
    }
}

func pagination_active_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: var(--chx-primary);
        color: var(--chx-primary-fg);
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
        border: 1px solid var(--chx-border);
        border-radius: 12px;
        background: var(--chx-surface);
        color: var(--chx-text-main);
    }
}

func table_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        border-collapse: collapse;
        overflow: hidden;
        border-radius: 14px;
        border: 1px solid var(--chx-border);
        background: var(--chx-surface);
    }
}

func table_head_cell_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.85rem 1rem;
        text-align: left;
        background: var(--chx-surface-2);
        color: var(--chx-text-main);
        font-size: 0.82rem;
        text-transform: uppercase;
        letter-spacing: 0.05em;
        border-bottom: 1px solid var(--chx-border);
    }
}

func table_cell_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.9rem 1rem;
        color: var(--chx-text-main);
        border-bottom: 1px solid var(--chx-border);
    }
}

public #universal Progress(props) {
    return <progress {...props} max={100} value={props.value} class={${progress_styles(page)}}></progress>
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

public #universal Tabs(props) {
    return <div {...props} class={${tabs_styles(page)}}>{props.children}</div>
}

public #universal TabList(props) {
    return <div {...props} class={${tab_list_styles(page)}}>{props.children}</div>
}

public #universal Tab(props) {
    return <div {...props} class={${tab_styles(page)}}>{props.children}</div>
}

public #universal TabActive(props) {
    return <Tab {...props} class={${tab_active_styles(page)}}>{props.children}</Tab>
}

public #universal TabPanel(props) {
    return <div {...props} class={${tab_panel_styles(page)}}>{props.children}</div>
}

public #universal Pagination(props) {
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
