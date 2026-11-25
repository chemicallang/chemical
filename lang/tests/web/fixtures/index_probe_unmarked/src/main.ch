// Unmarked module for scripts/web-index-test.sh (Phase 1 IndexModule probe).
// The universal plugin is loaded (universal_cbi is imported), so a
// `[index] universal_job_begin` line is expected, but no
// `universal_index_module` line may name this module.

public struct UnmarkedProps {
    var label : std::string_view
}

#universal UnmarkedWidget(props : UnmarkedProps) {
    return <div class="unmarked">{props.label}</div>
}

public func main() : int {
    return 0
}
