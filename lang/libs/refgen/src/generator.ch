
public namespace refgen {

// Find the comment token immediately preceding a given line number using binary search.
// We search the tokens vector for the last SingleLineComment or MultiLineComment
// whose position.line is just before node_line.
func find_comment_before(tokens : *mut VecRef<Token>, node_line : uint) : std::string_view {
    if (tokens == null) return std::string_view("", 0);
    var size = tokens.size();
    if (size == 0) return std::string_view("", 0);
    
    // Walk backwards from the token at or just before node_line
    // First, find the token closest to node_line via linear scan (binary search on position)
    var best_idx : uint = size; // invalid
    var i = 0u;
    while (i < size) {
        var tok = tokens.get(i);
        if (tok.position.line >= node_line) {
            break;
        }
        best_idx = i;
        i++;
    }
    
    if (best_idx == size) return std::string_view("", 0);
    
    // Now walk backward from best_idx looking for comment tokens
    // Comments must be immediately before the node (on the previous line or lines)
    var idx = best_idx;
    var comment_start = size; // invalid sentinel
    while (true) {
        var tok = tokens.get(idx);
        var tok_type = tok.type;
        // SingleLineComment = 140 (enum ordinal), MultiLineComment = 143
        if (tok_type == ChemicalTokenType.SingleLineComment as int || tok_type == ChemicalTokenType.MultiLineComment as int) {
            comment_start = idx;
            if (idx == 0) break;
            idx--;
        } else if (tok_type == ChemicalTokenType.NewLine as int || tok_type == ChemicalTokenType.Whitespace as int) {
            if (idx == 0) break;
            idx--;
        } else {
            break;
        }
    }
    
    if (comment_start == size) return std::string_view("", 0);
    
    // Return the value of the first comment token found (closest to the node)
    var comment_tok = tokens.get(comment_start);
    return comment_tok.value;
}

// Get a name from a node based on its kind
func get_node_name(node : *ASTNode) : std::string_view {
    var kind = node.getKind();
    if (kind == ASTNodeKind.FunctionDecl) {
        return (node as *FunctionDeclaration).getName();
    } else if (kind == ASTNodeKind.StructDecl) {
        return (node as *StructDefinition).getName();
    } else if (kind == ASTNodeKind.InterfaceDecl) {
        return (node as *InterfaceDefinition).getName();
    } else if (kind == ASTNodeKind.NamespaceDecl) {
        return (node as *Namespace).getName();
    } else if (kind == ASTNodeKind.EnumDecl) {
        return (node as *EnumDeclaration).getName();
    } else if (kind == ASTNodeKind.VariantDecl) {
        return (node as *VariantDefinition).getName();
    } else if (kind == ASTNodeKind.UnionDecl) {
        return (node as *UnionDef).getName();
    }
    return std::string_view("", 0);
}

// Get a kind label for documentation
func get_kind_label(kind : ASTNodeKind) : std::string_view {
    if (kind == ASTNodeKind.FunctionDecl) {
        return "function";
    } else if (kind == ASTNodeKind.StructDecl) {
        return "struct";
    } else if (kind == ASTNodeKind.InterfaceDecl) {
        return "interface";
    } else if (kind == ASTNodeKind.NamespaceDecl) {
        return "namespace";
    } else if (kind == ASTNodeKind.EnumDecl) {
        return "enum";
    } else if (kind == ASTNodeKind.VariantDecl) {
        return "variant";
    } else if (kind == ASTNodeKind.UnionDecl) {
        return "union";
    }
    return "declaration";
}

public struct Generator {
    var output_dir : std::string
    var ctx : *TransformerContext

    public func generate(&mut self, module : *TransformerModule) {
        var res = fs::create_dir_all(self.output_dir.data());
        if (res is std::Result.Err) {
            var Err(e) = res else unreachable;
            printf("Error creating output directory %s: %s\n", self.output_dir.data(), e.message().data());
            return;
        }

        var mod_name = module.getName();
        var mod_dir = self.output_dir.copy();
        mod_dir.append_view("/");
        mod_dir.append_view(mod_name);
        
        res = fs::create_dir_all(mod_dir.data());
        if (res is std::Result.Err) {
            var Err(e) = res else unreachable;
            printf("Error creating module directory %s: %s\n", mod_dir.data(), e.message().data());
            return;
        }

        var files = module.getFiles();
        if (files == null) return;

        var i = 0u;
        while (i < files.size()) {
            var file_meta = files.get(i);
            self.generate_file_docs(file_meta, mod_dir);
            i++;
        }
    }

    func generate_file_docs(&mut self, file_meta : *ASTFileMetaData, mod_dir : std::string) {
        var file_scope = file_meta.getFileScope();
        if (file_scope == null) return;

        var scope = file_scope.getBody();
        if (scope == null) return;

        var nodes = scope.getNodes();
        if (nodes == null) return;

        // Get tokens for this file to look up comments
        var file_id = file_meta.getFileId();
        var tokens = self.ctx.getFileTokens(file_id);

        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>");
        html.append_view(file_meta.getAbsPath());
        html.append_view("</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style></head><body>");

        html.append_view("<div class='header'>File: ");
        html.append_view(file_meta.getAbsPath());
        html.append_view("</div><div class='container'>");

        var i = 0u;
        while (i < nodes.size()) {
            var node = nodes.get(i);
            self.document_node(node, html, tokens);
            i++;
        }

        html.append_view("</div></body></html>");

        var out_file = mod_dir.copy();
        out_file.append_view("/");
        // Use file id as filename
        var file_id_str = std::string("");
        file_id_str.append_uinteger(file_id as ubigint);
        out_file.append_view(file_id_str.to_view());
        out_file.append_view(".html");

        var write_res = fs::write_text_file(out_file.data(), html.data() as *u8, html.size());
        if (write_res is std::Result.Err) {
            var Err(e) = write_res else unreachable;
            printf("Error writing documentation file %s: %s\n", out_file.data(), e.message().data());
        }
    }

    func document_node(&mut self, node : *ASTNode, html : &mut std::string, tokens : *mut VecRef<Token>) {
        var name = get_node_name(node);
        if (name.size() == 0) return;

        var kind = node.getKind();
        var kind_label = get_kind_label(kind);

        // Decode the node's source location to find its line number
        var encoded_loc = node.getEncodedLocation();
        var loc_data = self.ctx.decodeLocation(encoded_loc);
        var node_line = loc_data.lineStart;

        // Find comment immediately before this node
        var comment = find_comment_before(tokens, node_line);

        html.append_view("<div class='node'>");
        html.append_view("<span class='kind-badge'>");
        html.append_view(kind_label);
        html.append_view("</span> ");
        html.append_view("<span class='node-title'>");
        html.append_view(name);
        html.append_view("</span>");

        if (comment.size() > 0) {
            html.append_view("<div class='comment'>");
            html.append_view(comment);
            html.append_view("</div>");
        }

        html.append_view("</div>");
    }

    func get_css(&self) : std::string_view {
        return """
            :root {
                --bg: #030712;
                --bg-card: #0f172a;
                --border: #1e293b;
                --text: #f8fafc;
                --text-muted: #94a3b8;
                --accent: #38bdf8;
            }
            body { font-family: 'Inter', system-ui, sans-serif; background: var(--bg); color: var(--text); padding: 0; margin: 0; }
            .header { background: var(--bg-card); padding: 1rem 2rem; border-bottom: 1px solid var(--border); font-weight: 600; color: var(--accent); }
            .container { padding: 2rem; max-width: 900px; margin: 0 auto; }
            .node { background: var(--bg-card); border: 1px solid var(--border); padding: 1.5rem; margin-bottom: 1.5rem; border-radius: 12px; }
            .kind-badge { display: inline-block; background: rgba(56, 189, 248, 0.15); color: var(--accent); padding: 2px 10px; border-radius: 6px; font-size: 0.85rem; font-weight: 500; letter-spacing: 0.5px; vertical-align: middle; }
            .node-title { font-size: 1.25rem; font-weight: 700; color: var(--text); vertical-align: middle; }
            .comment { background: rgba(56, 189, 248, 0.05); padding: 1rem; border-radius: 6px; border-left: 4px solid var(--accent); font-size: 0.95rem; line-height: 1.6; color: var(--text-muted); white-space: pre-wrap; margin-top: 1rem; }
        """;
    }
}

}
