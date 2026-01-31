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
    var title = std::string();
    var link = std::string();
    var children = std::vector<*mut SummaryItem>();
    
    printf("  ListItem has %d children\n", item.children.size());
    
    if(item.children.size() > 0) {
        var i = 0u;
        while(i < item.children.size()) {
            var child = item.children.get(i);
            printf("    Child %d: kind=%d\n", i, child.kind);
            
            if(child.kind == md::MdNodeKind.Paragraph) {
                var p = child as *mut md::MdParagraph;
                var j = 0u;
                while(j < p.children.size()) {
                    var node = p.children.get(j);
                    if(node.kind == md::MdNodeKind.Link) {
                        var l = node as *mut md::MdLink;
                        link = std::string(l.url);
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
            } else if(child.kind == md::MdNodeKind.Link) {
                 var l = child as *mut md::MdLink;
                 link = std::string(l.url);
                 printf("    Found Link! URL='%s'\n", l.url.data());
                 var k = 0u;
                 while(k < l.children.size()) {
                    var lc = l.children.get(k);
                    if(lc.kind == md::MdNodeKind.Text) {
                         title.append_view((lc as *mut md::MdText).value);
                    }
                    k++;
                 }
            } else if(child.kind == md::MdNodeKind.Text) {
                 printf("    Found Text: '%s'\n", (child as *mut md::MdText).value.data());
                 title.append_view((child as *mut md::MdText).value);
            } else if(child.kind == md::MdNodeKind.List) {
                var l = child as *mut md::MdList;
                printf("    Found nested List with %d items\n", l.children.size());
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
    
    if(title.size() == 0 && children.size() == 0) return null;
    
    printf("  => Created SummaryItem: title='%s' link='%s' children=%d\n", title.c_str(), link.c_str(), children.size());
    
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
    
    printf("Parsing summary from: %s\n", path.data());
    var tokens = md::lex(view);
    printf("Lexed %d tokens\n", tokens.size());
    
    var root = md::parse(&tokens, &mut arena);
    if(root == null) {
        printf("Parse failed (root is null)\n");
        return null;
    }
    
    var summary = new Summary {
        title : std::string("Summary"),
        items : std::vector<*mut SummaryItem>()
    };
    
    // Traverse root
    var i = 0u;
    while(i < root.children.size()) {
        var node = root.children.get(i);
        // printf("Node kind: %d\n", node.kind);
        if(node.kind == md::MdNodeKind.Header) {
            // Header
        } else if(node.kind == md::MdNodeKind.List) {
            var list = node as *mut md::MdList;
            printf("Found List with %d items\n", list.children.size());
            var j = 0u;
            while(j < list.children.size()) {
                var item = parse_list_item(list.children.get(j) as *mut md::MdListItem, &mut arena);
                if(item != null) {
                    summary.items.push_back(item);
                } else {
                    printf("Failed to parse list item %d\n", j);
                }
                j++;
            }
        }
        i++;
    }
    
    printf("Parsed summary with %d top-level items.\n", summary.items.size());
    return summary;
}

}
