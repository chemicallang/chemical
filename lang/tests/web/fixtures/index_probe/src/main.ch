// Marked module for scripts/web-index-test.sh (Phase 1 IndexModule probe).
// The gate asserts `universal_index_module` is called exactly once for this
// module with a non-zero declaration count that includes this struct and
// function.

public struct ProbeWidgetProps {
    var label : std::string_view
}

public func probe_helper() : int {
    return 7
}

#universal ProbeWidget(props : ProbeWidgetProps) {
    return <div class="probe">{props.label}</div>
}

public func main() : int {
    return 0
}
