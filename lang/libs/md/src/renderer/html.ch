public namespace md {

public func render_node(r : &mut HtmlRenderer, node : *mut MdNode) {
    if(node == null) return;
    
    // Try inline first
    if(render_inline_node(r, node)) return;
    
    // Try block
    if(render_block_node(r, node)) return;
    
    // Fallback? Or specific types not handled?
    // utils.ch/render_children calls this.
}

public func render_to_html(root : *mut MdRoot) : std::string {
    if(root == null) return std::string();
    var r = HtmlRenderer { out : std::string() };
    render_node(r, root as *mut MdNode);
    // Return r.out, but we need to ensure we return a new string or move it?
    // r.out is std::string.
    return std::replace(r.out, std::string());
}

}
