func paper_styles(page : &mut HtmlPage) : *char {
    return #css {
        background: linear-gradient(180deg, rgba(255, 255, 255, 0.03), transparent 55%), hsl(var(--background));
        border: 1px solid hsl(var(--border));
        border-radius: var(--radius);
        box-shadow: var(--shadow);
        padding: 1.25rem;
    }
}

func appbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 1rem;
        padding: 0.9rem 1.15rem;
        border-radius: 18px;
        border: 1px solid hsl(var(--border));
        background: rgba(15, 23, 42, 0.04);
        box-shadow: var(--shadow-sm);
    }
}

func drawer_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.85rem;
        padding: 1.1rem;
        border: 1px solid hsl(var(--border));
        border-radius: var(--radius);
        background: hsl(var(--background));
        box-shadow: var(--shadow);
    }
}

func menu_styles(page : &mut HtmlPage) : *char {
    // Portaled into document.body and positioned fixed by $__uni_floating;
    // visibility is driven by data-open (inline style would wipe positioning).
    return #css {
        min-width: 220px;
        position: fixed;
        top: 0;
        left: 0;
        z-index: 20;
        display: none;
        gap: 0.35rem;
        padding: 0.55rem;
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        box-shadow: var(--shadow);
        &[data-open="true"] {
            display: grid;
        }
    }
}

func menu_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.7rem 0.85rem;
        border-radius: 10px;
        color: hsl(var(--foreground));
        text-decoration: none;
        transition: background 0.18s ease, color 0.18s ease;
        &:hover {
            background: hsl(var(--muted));
        }
    }
}

func popover_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 280px;
        max-width: 100%;
        display: grid;
        gap: 0.55rem;
        padding: 0.9rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 14px;
        background: hsl(var(--background));
        box-shadow: var(--shadow);
        z-index: 50;
        animation: chx-slide-down 0.15s var(--ease);
    }
}

func popover_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.125rem;
    }
}

func popover_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.875rem;
        font-weight: 600;
        line-height: 1.25rem;
        color: hsl(var(--foreground));
        margin: 0;
    }
}

func popover_desc_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.8125rem;
        color: hsl(var(--muted-foreground));
        margin: 0;
    }
}

func dialog_overlay_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: fixed;
        inset: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 1.5rem;
        z-index: 50;
    }
}

func dialog_backdrop_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        inset: 0;
        background: rgba(2, 6, 23, 0.62);
        backdrop-filter: blur(6px);
    }
}

func dialog_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 420px;
        max-width: 100%;
        max-height: calc(100vh - 3rem);
        position: relative;
        z-index: 1;
        border: 1px solid hsl(var(--border));
        border-radius: 20px;
        padding: 1.5rem;
        background: hsl(var(--background));
        box-shadow: var(--shadow-lg);
        color: hsl(var(--foreground));
        display: flex;
        flex-direction: column;
        &[data-size="sm"] { width: 360px; padding: 1.25rem; }
        &[data-size="lg"] { width: 540px; }
        &[data-scrollable="true"] {
            overflow: hidden;
        }
    }
}

func dialog_header_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.375rem;
        text-align: left;
        flex-shrink: 0;
    }
}

func dialog_title_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 1.125rem;
        font-weight: 600;
        line-height: 1.5rem;
        margin: 0;
        color: hsl(var(--foreground));
    }
}

func dialog_desc_styles(page : &mut HtmlPage) : *char {
    return #css {
        font-size: 0.875rem;
        color: hsl(var(--muted-foreground));
        margin: 0;
    }
}

func dialog_body_styles(page : &mut HtmlPage) : *char {
    return #css {
        overflow-y: auto;
        flex: 1;
        min-height: 0;
        padding: 1rem 0;
    }
}

func dialog_footer_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.5rem;
        flex-shrink: 0;
        padding-top: 1rem;
    }
}

func dialog_close_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        top: 1rem;
        right: 1rem;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 1.5rem;
        height: 1.5rem;
        border-radius: 9999px;
        border: 0;
        background: transparent;
        color: hsl(var(--muted-foreground));
        cursor: pointer;
        padding: 0;
        transition: color 0.15s ease, background 0.15s ease;
        z-index: 1;
        &:hover {
            color: hsl(var(--foreground));
            background: hsl(var(--muted));
        }
        &:focus-visible {
            outline: 2px solid hsl(var(--ring));
            outline-offset: 2px;
        }
    }
}

func dialog_actions_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        justify-content: flex-end;
        gap: 0.5rem;
        flex-shrink: 0;
        padding-top: 1rem;
    }
}

func snackbar_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        gap: 0.8rem;
        padding: 0.85rem 1rem;
        border-radius: 999px;
        border: 1px solid hsl(var(--border));
        background: hsl(var(--background));
        color: hsl(var(--foreground));
        box-shadow: var(--shadow);
    }
}

func tooltip_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        padding: 0.375rem 0.625rem;
        border-radius: calc(var(--radius) - 2px);
        background: hsl(var(--primary));
        color: hsl(var(--primary-foreground));
        font-size: 0.75rem;
        font-weight: 500;
        line-height: 1.4;
        box-shadow: var(--shadow-md);
        pointer-events: none;
        white-space: nowrap;
        z-index: 50;
        animation: chx-fade-in 0.1s ease;
    }
}

func tooltip_arrow_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: absolute;
        width: 8px;
        height: 8px;
        background: hsl(var(--primary));
        transform: rotate(45deg);
    }
}

func bottom_bar_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 100%;
        max-width: 420px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 0.75rem;
        padding: 0.85rem 1rem;
        border: 1px solid hsl(var(--border));
        border-radius: 999px;
        background: rgba(255, 255, 255, 0.85);
        box-shadow: var(--shadow-lg);
        backdrop-filter: blur(18px);
    }
}

func empty_state_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.75rem;
        justify-items: start;
        padding: 1.3rem;
        border: 1px dashed hsl(var(--border));
        border-radius: var(--radius);
        background: linear-gradient(180deg, rgba(59, 130, 246, 0.04), transparent 60%), hsl(var(--background));
    }
}

func stat_card_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: grid;
        gap: 0.5rem;
        padding: 1.15rem;
        border: 1px solid hsl(var(--border));
        border-radius: 18px;
        background: hsl(var(--background));
        box-shadow: var(--shadow-sm);
    }
}

func icon_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 2rem;
        height: 2rem;
        border-radius: 999px;
        background: hsl(var(--muted));
        border: 1px solid hsl(var(--border));
        color: hsl(var(--foreground));
        font-weight: 700;
        line-height: 1;
        font-family: ui-monospace, "SFMono-Regular", monospace;
    }
}

public #universal Paper(props) {
    return <section {...props} class={${paper_styles(page)}}>{props.children}</section>
}

public #universal AppBar(props) {
    return <header {...props} class={${appbar_styles(page)}}>{props.children}</header>
}

public #universal Drawer(props) {
    return <aside {...props} class={${drawer_styles(page)}}>{props.children}</aside>
}

public #universal Menu(props) {
    return <div {...props} role="menu" class={${menu_styles(page)}}>{props.children}</div>
}

public #universal MenuItem(props) {
    return <a {...props} role="menuitem" class={${menu_item_styles(page)}}>{props.children}</a>
}

// Shadcn-style Popover with trigger + content composition.
// Modes:
// 1. Trigger mode: <Popover trigger={<Button>Open</Button>}><PopoverHeader>...</PopoverHeader></Popover>
// 2. Simple mode: <Popover>content here</Popover>
// Props: open, defaultOpen, onClose, side (top/bottom/left/right), align (start/center/end), className
public #universal Popover(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    const triggerRef = useRef(null)
    const contentRef = useRef(null)
    var close = () => {
        open = false
        if(props.onClose) { props.onClose() }
    }
    var toggle = () => {
        if(props.open != null) {
            if(props.onClose) { props.onClose() }
        } else {
            open = !open
        }
    }
    useEffect(() => {
        const handler = (e) => { if(e.key === "Escape") { close() } }
        document.addEventListener("keydown", handler)
        return () => document.removeEventListener("keydown", handler)
    }, [])
    // Position the popover under the trigger using floating UI
    useEffect(() => {
        if(!isOpen || !triggerRef.current || !contentRef.current) { return () => {} }
        return window.$__uni_floating(triggerRef.current, contentRef.current, { gap: 8 })
    }, [isOpen])
    // Click outside to close
    useEffect(() => {
        if(!isOpen) { return () => {} }
        const handler = (e) => {
            if(contentRef.current && !contentRef.current.contains(e.target) && triggerRef.current && !triggerRef.current.contains(e.target)) {
                close()
            }
        }
        document.addEventListener("mousedown", handler)
        return () => document.removeEventListener("mousedown", handler)
    }, [isOpen])
    var classes = (props.className || props.class) || ""
    // Trigger mode
    if(props.trigger) {
        return <div style="position:relative;display:inline-block;">
            <span ref={triggerRef} onClick={toggle} aria-haspopup="dialog" aria-expanded={isOpen ? "true" : "false"}>{props.trigger}</span>
            {createPortal(
                <div ref={contentRef} class={${popover_styles(page)}} class={classes} style={isOpen ? "" : "display:none;"} role="dialog" aria-label={props.ariaLabel}>
                    {props.children}
                </div>
            )}
        </div>
    }
    // Simple mode (backward compat)
    return <div {...props} class={${popover_styles(page)}} class={classes}>{props.children}</div>
}

public #universal PopoverTrigger(props) {
    return <button {...props} type="button">{props.children}</button>
}

public #universal PopoverContent(props) {
    return <div {...props} class={${popover_styles(page)}}>{props.children}</div>
}

public #universal PopoverHeader(props) {
    return <div {...props} class={${popover_header_styles(page)}}>{props.children}</div>
}

public #universal PopoverTitle(props) {
    return <h3 {...props} class={${popover_title_styles(page)}}>{props.children}</h3>
}

public #universal PopoverDescription(props) {
    return <p {...props} class={${popover_desc_styles(page)}}>{props.children}</p>
}

// Modal dialog in two modes (shadcn-style `open`/`onOpenChange`):
// - Controlled: pass `open={...}` + `onClose`; visibility tracks the prop
//   reactively, so parent state drives show/hide.
// - Uncontrolled: `defaultOpen` sets the initial (and SSR) state; backdrop
//   click and the Escape key dismiss via internal state.
// Both modes fire `props.onClose` on dismissal.
public #universal Dialog(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    const contentRef = useRef(null)
    var close = () => {
        open = false
        if(props.onClose) {
            props.onClose()
        }
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
    // Focus management (WAI-ARIA dialog pattern): on open, remember the
    // trigger, move focus to the first focusable element inside the dialog and
    // trap Tab/Shift+Tab within it. On close, restore focus to the trigger.
    useEffect(() => {
        if(!isOpen) {
            return () => {}
        }
        const dialogEl = contentRef.current
        if(!dialogEl) {
            return () => {}
        }
        const previouslyFocused = document.activeElement
        const focusables = () => dialogEl.querySelectorAll('button:not([disabled]), a[href], input:not([disabled]), select:not([disabled]), textarea:not([disabled]), [tabindex]:not([tabindex="-1"])')
        const firstEl = () => {
            const list = focusables()
            return list.length > 0 ? list[0] : null
        }
        const lastEl = () => {
            const list = focusables()
            return list.length > 0 ? list[list.length - 1] : null
        }
        const focusFirst = () => {
            const el = firstEl()
            if(el) { el.focus() }
        }
        const onKeyDown = (e) => {
            if(e.key !== "Tab") {
                return
            }
            const list = focusables()
            if(list.length === 0) {
                e.preventDefault()
                return
            }
            const active = document.activeElement
            if(e.shiftKey) {
                if(active === firstEl() || !dialogEl.contains(active)) {
                    e.preventDefault()
                    const last = lastEl()
                    if(last) { last.focus() }
                }
            } else {
                if(active === lastEl() || !dialogEl.contains(active)) {
                    e.preventDefault()
                    const first = firstEl()
                    if(first) { first.focus() }
                }
            }
        }
        document.addEventListener("keydown", onKeyDown)
        focusFirst()
        return () => {
            document.removeEventListener("keydown", onKeyDown)
            if(previouslyFocused && previouslyFocused.focus) {
                previouslyFocused.focus()
            }
        }
    }, [isOpen])
    // WAI-ARIA modal pattern: while the dialog is open the background becomes
    // inert (not focusable/clickable for AT and keyboard users). The modal flag
    // marks this portal as a modal overlay for the inert manager; the effect
    // re-scans on open/close (the overlay hides via display:none when closed).
    useEffect(() => {
        window.$__uni_inert_scan();
        return () => window.$__uni_inert_scan();
    }, [isOpen])
    // The overlay renders into document.body via createPortal so position: fixed
    // is viewport-relative even inside transform/overflow ancestors (shadcn does
    // the same). SSR renders it inline; hydration moves it to body.
    return createPortal(
        <div {...props} class={${dialog_overlay_styles(page)}} style={isOpen ? "" : "display:none;"}>
            <DialogBackdrop onClick={close}></DialogBackdrop>
            <DialogContent ref={contentRef} role="dialog" aria-modal="true" aria-label={props.ariaLabel}>{props.children}</DialogContent>
        </div>,
        { modal: true }
    )
}

public #universal DialogBackdrop(props) {
    return <div {...props} class={${dialog_backdrop_styles(page)}}></div>
}

public #universal DialogContent(props) {
    var showClose = props.showCloseButton != null ? props.showCloseButton : true
    var scrollable = props.scrollable || false
    var size = props.size || "default"
    var classes = (props.className || props.class) || ""
    return <div {...props} class={${dialog_content_styles(page)}} class={classes} data-size={size} data-scrollable={scrollable ? "true" : "false"}>
        {showClose ? <button type="button" class={${dialog_close_styles(page)}} onClick={props.onClose} aria-label="Close"><svg width="15" height="15" viewBox="0 0 15 15" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M11.7816 4.03157C12.0062 3.80702 12.0062 3.44295 11.7816 3.2184C11.5571 2.99385 11.193 2.99385 10.9685 3.2184L7.50005 6.68682L4.03164 3.2184C3.80708 2.99385 3.44301 2.99385 3.21846 3.2184C2.99391 3.44295 2.99391 3.80702 3.21846 4.03157L6.68688 7.49999L3.21846 10.9684C2.99391 11.193 2.99391 11.557 3.21846 11.7816C3.44301 12.0061 3.80708 12.0061 4.03164 11.7816L7.50005 8.31316L10.9685 11.7816C11.193 12.0061 11.5571 12.0061 11.7816 11.7816C12.0062 11.557 12.0062 11.193 11.7816 10.9684L8.31322 7.49999L11.7816 4.03157Z" fill="currentColor" fill-rule="evenodd" clip-rule="evenodd"></path></svg></button> : null}
        {props.children}
    </div>
}

public #universal DialogHeader(props) {
    return <div {...props} class={${dialog_header_styles(page)}}>{props.children}</div>
}

public #universal DialogTitle(props) {
    var classes = (props.className || props.class) || ""
    return <h2 {...props} class={${dialog_title_styles(page)}} class={classes}>{props.children}</h2>
}

public #universal DialogDescription(props) {
    var classes = (props.className || props.class) || ""
    return <p {...props} class={${dialog_desc_styles(page)}} class={classes}>{props.children}</p>
}

public #universal DialogBody(props) {
    return <div {...props} class={${dialog_body_styles(page)}}>{props.children}</div>
}

public #universal DialogFooter(props) {
    var classes = (props.className || props.class) || ""
    return <div {...props} class={${dialog_footer_styles(page)}} class={classes}>{props.children}</div>
}

// Legacy alias
public #universal DialogActions(props) {
    return <DialogFooter {...props}>{props.children}</DialogFooter>
}

public #universal Snackbar(props) {
    return <div {...props} role="status" class={${snackbar_styles(page)}}>{props.children}</div>
}

// Shadcn-style Tooltip: shows a tooltip bubble on hover/focus.
// Props:
//   content   tooltip text or JSX (shadcn uses children on TooltipContent)
//   label     alias for content (backward compat)
//   side      "top" (default) | "bottom" | "left" | "right"
//   delay     hover delay in ms (default 200)
//   className merged with the generated style class
//   children  the trigger element
//
// TooltipProvider is not needed — each Tooltip manages its own timing.
// 
// Usage:
//   <Tooltip content="Add to library"><Button>Hover</Button></Tooltip>
//   <Tooltip label="Save" side="right"><IconButton /></Tooltip>
public #universal Tooltip(props) {
    state visible = false
    state hoverTimeout = null
    state leaveTimeout = null
    var tipContent = props.content || props.label || ""
    var side = props.side || "top"
    var delay = props.delay || 200
    var classes = (props.className || props.class) || ""

    if(tipContent != "") {
        var show = () => {
            if(leaveTimeout) { clearTimeout(leaveTimeout); leaveTimeout = null }
            hoverTimeout = setTimeout(() => { visible = true }, delay)
        }
        var hide = () => {
            if(hoverTimeout) { clearTimeout(hoverTimeout); hoverTimeout = null }
            visible = false
        }
        // Position the tooltip relative to the trigger using CSS transforms
        var tipStyle = "position:absolute;z-index:50;pointer-events:none;white-space:nowrap;"
        if(side == "bottom") {
            tipStyle = tipStyle + "top:calc(100% + 8px);left:50%;transform:translateX(-50%);"
        } else if(side == "left") {
            tipStyle = tipStyle + "right:calc(100% + 8px);top:50%;transform:translateY(-50%);"
        } else if(side == "right") {
            tipStyle = tipStyle + "left:calc(100% + 8px);top:50%;transform:translateY(-50%);"
        } else {
            tipStyle = tipStyle + "bottom:calc(100% + 8px);left:50%;transform:translateX(-50%);"
        }
        if(!visible) { tipStyle = tipStyle + "opacity:0;visibility:hidden;" }
        else { tipStyle = tipStyle + "opacity:1;visibility:visible;" }
        return <span style="position:relative;display:inline-flex;" onMouseEnter={show} onMouseLeave={hide} onFocus={show} onBlur={hide}>
            {props.children}
            <span role="tooltip" class={${tooltip_styles(page)} + " " + classes} style={tipStyle}>{tipContent}</span>
        </span>
    }
    return <span {...props} class={${tooltip_styles(page)} + " " + classes}>{props.children}</span>
}

// TooltipProvider: wraps the app to configure global tooltip behavior.
// delay: global hover delay override (ms)
public #universal TooltipProvider(props) {
    return <span {...props}>{props.children}</span>
}

// TooltipContent: wraps tooltip text content (shadcn API compatibility)
public #universal TooltipContent(props) {
    return <span {...props}>{props.children}</span>
}

// TooltipTrigger: wraps the trigger element (shadcn API compatibility)
public #universal TooltipTrigger(props) {
    return <span {...props}>{props.children}</span>
}

public #universal Icon(props) {
    return <span {...props} class={${icon_styles(page)}}>{props.children}</span>
}

public #universal BottomBar(props) {
    return <nav {...props} class={${bottom_bar_styles(page)}}>{props.children}</nav>
}

public #universal EmptyState(props) {
    return <section {...props} class={${empty_state_styles(page)}}>{props.children}</section>
}

public #universal StatCard(props) {
    return <section {...props} class={${stat_card_styles(page)}}>{props.children}</section>
}

// Dropdown menu in two modes (shadcn-style `open`/`onOpenChange`):
// - Controlled: pass `open={...}` + `onClose`; visibility tracks the prop
//   reactively (parent state drives open/close).
// - Uncontrolled: `defaultOpen` sets the initial (and SSR) state; the
//   trigger toggles it.
// A fixed overlay behind the menu closes it on outside click; the Escape key
// dismisses too. `props.trigger` may be text or JSX; children are items.
// `props.menuStyle`/`props.itemClassName` allow per-use styling tweaks.
public #universal Dropdown(props) {
    state open = props.defaultOpen ? true : false
    var isOpen = props.open != null ? props.open : open
    const triggerRef = useRef(null)
    const menuRef = useRef(null)
    var close = () => {
        open = false
        if(props.onClose) {
            props.onClose()
        }
    }
    var toggle = () => {
        if(props.open != null) {
            if(props.onToggle) {
                props.onToggle()
            }
        } else {
            open = !open
        }
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
    // Portaled into document.body and anchored under the trigger by
    // $__uni_floating, so the menu escapes overflow/transform clipping.
    useEffect(() => {
        if(!isOpen || !triggerRef.current || !menuRef.current) {
            return () => {}
        }
        return window.$__uni_floating(triggerRef.current, menuRef.current, { gap: 8 })
    }, [isOpen])
    var menuStyle = ""
    if(props.menuStyle) {
        menuStyle = props.menuStyle
    }
    return <div style="position:relative;display:inline-block;">
        <div onClick={close} style={isOpen ? "position:fixed;inset:0;z-index:5;" : "display:none;"}></div>
        <Button ref={triggerRef} onClick={toggle} aria-haspopup="menu" aria-expanded={isOpen ? "true" : "false"}>{props.trigger}</Button>
        {createPortal(
            <Menu ref={menuRef} data-open={isOpen ? "true" : "false"} style={menuStyle}>
                {props.children}
            </Menu>
        )}
    </div>
}

public #universal DropdownItem(props) {
    return <MenuItem {...props}>{props.children}</MenuItem>
}

// Shadcn-style DropdownMenu: composition pattern with Trigger, Content, Item, etc.
// Usage:
//   <DropdownMenu>
//       <DropdownMenuTrigger render={<Button />}>Open</DropdownMenuTrigger>
//       <DropdownMenuContent>
//           <DropdownMenuItem>Profile</DropdownMenuItem>
//           <DropdownMenuSeparator />
//           <DropdownMenuItem>Logout</DropdownMenuItem>
//       </DropdownMenuContent>
//   </DropdownMenu>
func dropdown_content_styles(page : &mut HtmlPage) : *char {
    return #css {
        min-width: 220px;
        position: fixed;
        top: 0;
        left: 0;
        z-index: 50;
        display: none;
        gap: 0.25rem;
        padding: 0.375rem;
        border: 1px solid hsl(var(--border));
        border-radius: calc(var(--radius) - 2px);
        background: hsl(var(--popover));
        color: hsl(var(--popover-foreground));
        box-shadow: var(--shadow-md);
        animation: chx-slide-down 0.15s var(--ease);
        &[data-open="true"] {
            display: grid;
        }
    }
}

func dropdown_item_styles(page : &mut HtmlPage) : *char {
    return #css {
        display: flex;
        align-items: center;
        gap: 0.5rem;
        width: 100%;
        padding: 0.375rem 0.5rem;
        border-radius: calc(var(--radius) - 2px);
        border: none;
        background: transparent;
        color: hsl(var(--popover-foreground));
        font-size: 0.875rem;
        text-align: left;
        cursor: pointer;
        user-select: none;
        transition: background-color 0.1s ease;
        outline: none;
        &:hover, &[data-highlighted="true"] {
            background: hsl(var(--accent));
            color: hsl(var(--accent-foreground));
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            pointer-events: none;
        }
        svg { flex-shrink: 0; opacity: 0.6; }
    }
}

func dropdown_check_styles(page : &mut HtmlPage) : *char {
    return #css {
        width: 1rem;
        height: 1rem;
        flex-shrink: 0;
    }
}

func dropdown_separator_styles(page : &mut HtmlPage) : *char {
    return #css {
        height: 1px;
        background: hsl(var(--border));
        margin: 0.25rem -0.25rem;
    }
}

func dropdown_label_styles(page : &mut HtmlPage) : *char {
    return #css {
        padding: 0.375rem 0.5rem;
        font-size: 0.75rem;
        font-weight: 600;
        color: hsl(var(--foreground));
    }
}

func dropdown_shortcut_styles(page : &mut HtmlPage) : *char {
    return #css {
        margin-left: auto;
        font-size: 0.75rem;
        letter-spacing: 0.05em;
        color: hsl(var(--muted-foreground));
    }
}

// DropdownMenu: wraps a trigger + content for context menu behavior.
// Uses Dropdown (existing) under the hood. Just renders children.
public #universal DropdownMenu(props) {
    return <span {...props}>{props.children}</span>
}

public #universal DropdownMenuTrigger(props) {
    return <span {...props} onClick={props.onClick}>{props.children}</span>
}

public #universal DropdownMenuContent(props) {
    return <div {...props} class={${dropdown_content_styles(page)}} role="menu">{props.children}</div>
}

public #universal DropdownMenuItem(props) {
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <button type="button" role="menuitem" data-disabled={disabled ? "true" : "false"} class={classes + " " + ${dropdown_item_styles(page)}} onClick={props.onClick} disabled={disabled}>{props.children}</button>
}

public #universal DropdownMenuSeparator(props) {
    return <div role="separator" class={${dropdown_separator_styles(page)}}></div>
}

public #universal DropdownMenuLabel(props) {
    return <div class={${dropdown_label_styles(page)}}>{props.children}</div>
}

public #universal DropdownMenuShortcut(props) {
    return <span class={${dropdown_shortcut_styles(page)}}>{props.children}</span>
}

public #universal DropdownMenuCheckboxItem(props) {
    var checked = props.checked || false
    var disabled = props.disabled || false
    var classes = (props.className || props.class) || ""
    return <button type="button" role="menuitemcheckbox" aria-checked={checked ? "true" : "false"} data-disabled={disabled ? "true" : "false"} class={classes + " " + ${dropdown_item_styles(page)}} onClick={props.onClick} disabled={disabled}>
        <span class={${dropdown_check_styles(page)}}>{checked ? "✓" : ""}</span>
        {props.children}
    </button>
}
