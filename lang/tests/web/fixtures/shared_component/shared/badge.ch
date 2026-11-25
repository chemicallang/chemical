public struct BadgeProps {
    var label : std::string_view
}

// The shared component. `document` is read inside a lambda (handler position), so
// the hydration check is exempt and only the client-side ambient-root check can
// see it. The client-only provider supplies `document`; the server job skips the
// body check but must still resolve this component from the render site below.
#universal SharedBadge(props : BadgeProps) {
    var on_click = () => { console.log(document.title) }
    return <span class="shared-badge" onClick={on_click}>{props.label}</span>
}

// Rendered by the server entry point through `#html`, which goes through
// html_cbi's `symbol is not a component` check.
public func render_shared_badge(page : &mut HtmlPage) {
    #html {
        <SharedBadge label="from-shared" />
    }
}
