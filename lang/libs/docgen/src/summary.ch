public namespace docgen {

public struct SummaryItem {
    var title : std::string
    var link : std::string // Empty if section header only
    var children : std::vector<*mut SummaryItem>
}

public struct Summary {
    var title : std::string
    var items : std::vector<*mut SummaryItem>
}

// Traverse MD AST to build Summary
// SUMMARY.md structure is:
// # Title
// - [Link](url)
//     - [SubLink](url)
// - Section
//     - [Link](url)

func parse_list_item(item : *mut md::MdListItem, arena : *mut md::Arena) : *mut SummaryItem {
    // Expect first child to be Paragraph? Or directly content?
    // MdListItem children are block nodes. Usually a Paragraph.
    
    var title = std::string();
    var link = std::string();
    var children = std::vector<*mut SummaryItem>();
    
    if(item.children.size() > 0) {
        // Iterate children
        var i = 0u;
        while(i < item.children.size()) {
            var child = item.children.get(i);
            if(child.kind == md::MdNodeKind.Paragraph) {
                // Extract link/title from paragraph
                var p = child as *mut md::MdParagraph;
                var j = 0u;
                while(j < p.children.size()) {
                    var node = p.children.get(j);
                    if(node.kind == md::MdNodeKind.Link) {
                        var l = node as *mut md::MdLink;
                        link = std::string(l.url);
                        // Extract text from children
                        var k = 0u;
                        while(k < l.children.size()) {
                            var lc = l.children.get(k);
                            if(lc.kind == md::MdNodeKind.Text) {
                                title.append_view((lc as *mut md::MdText).value);
                            }
                            k++;
                        }
                    } else if(node.kind == md::MdNodeKind.Text) {
                        title.append_view((node as *mut md::MdText).value);
                    }
                    j++;
                }
            } else if(child.kind == md::MdNodeKind.List) {
                // Nested list
                var l = child as *mut md::MdList;
                var j = 0u;
                while(j < l.children.size()) {
                    var subitem = parse_list_item(l.children.get(j) as *mut md::MdListItem, arena);
                    if(subitem != null) children.push_back(subitem);
                    j++;
                }
            }
            i++;
        }
    }
    
    // Allocate SummaryItem (not using arena for these, using heap/vector ownership for simplicity in docgen)
    // Or we should manage memory? Vector of pointers.
    // We will use new/malloc or just return struct? Struct pointers in vector.
    
    if(title.size() == 0 && children.size() == 0) return null;
    
    var summary_item = new SummaryItem {
        title : title,
        link : link,
        children : children
    };
    return summary_item;
}

public func parse_summary(path : std::string_view) : *mut Summary {
    var result = fs::read_entire_file(path.data());
    if(result is std.Result.Err) return null;
    
    var Ok(content) = result else unreachable;
    var view = std::string_view(content.data() as *char, content.size());
    
    var arena = md::Arena();
    
    var tokens = md::lex(view);
    var root = md::parse(&tokens, &mut arena);
    if(root == null) return null;
    
    var summary = new Summary {
        title : std::string("Summary"),
        items : std::vector<*mut SummaryItem>()
    };
    
    // Traverse root
    var i = 0u;
    while(i < root.children.size()) {
        var node = root.children.get(i);
        if(node.kind == md::MdNodeKind.Header) {
            // Main title?
            // User said: # Summary
        } else if(node.kind == md::MdNodeKind.List) {
            var list = node as *mut md::MdList;
            var j = 0u;
            while(j < list.children.size()) {
                var item = parse_list_item(list.children.get(j) as *mut md::MdListItem, &mut arena);
                if(item != null) summary.items.push_back(item);
                j++;
            }
        }
        i++;
    }
    
    return summary;
}

}
