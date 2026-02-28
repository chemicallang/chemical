
public namespace refgen {

func find_comment_before(tokens : std::span<Token>, node_line : uint) : std::string_view {
    var size = tokens.size();
    if (size == 0) return std::string_view();
    
    var best_idx : uint = size;
    var i = 0u;
    while (i < size) {
        var tok = tokens.data() + i;
        if (tok.position.line >= node_line) break;
        best_idx = i;
        i++;
    }
    
    if (best_idx == size) return std::string_view();
    
    var idx = best_idx;
    var comment_start = size;
    while (true) {
        var tok = tokens.data() + idx;
        var tok_type = tok.type;
        // Check for comment tokens (these values should be double checked against ChemicalTokenType)
        if (tok_type == 140 || tok_type == 143) {
            comment_start = idx;
            if (idx == 0) break;
            idx--;
        } else if (tok_type == 144 || tok_type == 145 || tok_type == 146) { // Whitespace/Newline
            if (idx == 0) break;
            idx--;
        } else {
            break;
        }
        // Don't go back too far (e.g. more than 3 lines of gap)
        if (node_line - tok.position.line > 5) break;
    }
    
    if (comment_start >= size) return std::string_view();
    return tokens.get(comment_start).value;
}

func clean_comment(comment : std::string_view) : std::string {
    var s = std::string("");
    var data = comment.data();
    var len = comment.size();
    
    // Very basic cleaning: skip //, /*, /**, */ and leading *
    var i = 0u;
    while (i < len) {
        if (data[i] == '/' && i + 1 < len && (data[i+1] == '/' || data[i+1] == '*')) {
            i += 2;
            if (i < len && data[i] == '*') i++; // match /**
            continue;
        }
        if (data[i] == '*' && i + 1 < len && data[i+1] == '/') {
            i += 2;
            continue;
        }
        // Skip leading * on lines
        if (data[i] == '*' && (i == 0 || data[i-1] == '\n' || (i > 0 && data[i-1] == ' '))) {
            i++;
            if (i < len && data[i] == ' ') i++;
            continue;
        }
        
        s.append(data[i]);
        i++;
    }
    return s;
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
    return std::string_view();
}

// Get a kind label for documentation
func get_kind_label(kind : ASTNodeKind) : std::string_view {
    if (kind == ASTNodeKind.FunctionDecl) {
        return std::string_view("function");
    } else if (kind == ASTNodeKind.StructDecl) {
        return std::string_view("struct");
    } else if (kind == ASTNodeKind.InterfaceDecl) {
        return std::string_view("interface");
    } else if (kind == ASTNodeKind.NamespaceDecl) {
        return std::string_view("namespace");
    } else if (kind == ASTNodeKind.EnumDecl) {
        return std::string_view("enum");
    } else if (kind == ASTNodeKind.VariantDecl) {
        return std::string_view("variant");
    } else if (kind == ASTNodeKind.UnionDecl) {
        return std::string_view("union");
    }
    return std::string_view("declaration");
}

public struct SymbolInfo {
    var name : std::string
    var kind : ASTNodeKind
    var file_id : uint
    var mod_name : std::string
    var access : AccessSpecifier
    var encoded_loc : ubigint
}

public struct Generator {
    var output_dir : std::string
    var ctx : *TransformerContext
    var github_links : std::string
    var no_search : bool
    var index : std::vector<SymbolInfo>

    public func index_module(&mut self, module : *TransformerModule) {
        var mod_name = module.getName();
        var count = module.getFileCount();
        for (var i = 0u; i < count; i++) {
            var file_meta = module.getFile(i);
            var file_id = file_meta.getFileId();
            var file_scope = file_meta.getFileScope();
            if (file_scope == null) continue;
            var scope = file_scope.getBody();
            if (scope == null) continue;
            var nodes = scope.getNodes();
            if (nodes == null) continue;
            
            for (var j = 0u; j < nodes.size(); j++) {
                var node = nodes.get(j);
                var name = get_node_name(node);
                if (name.size() == 0) continue;
                
                self.index.push_back(SymbolInfo {
                    name = std::string(name.data(), name.size()),
                    kind = node.getKind(),
                    file_id = file_id,
                    mod_name = std::string(mod_name.data(), mod_name.size()),
                    access = node.getAccessSpecifier(),
                    encoded_loc = node.getEncodedLocation()
                });
            }
        }
    }

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

        var count = module.getFileCount();
        var i = 0u;
        while (i < count) {
            var file_meta = module.getFile(i);
            self.generate_file_docs(file_meta, mod_dir.to_view(), mod_name);
            i++;
        }
    }

    public func finish(&mut self) {
        self.generate_index_html();
        if (!self.no_search) {
            self.generate_search_index();
        }
    }

    func generate_file_docs(&mut self, file_meta : *ASTFileMetaData, mod_dir : std::string_view, mod_name : std::string_view) {
        var file_scope = file_meta.getFileScope();
        if (file_scope == null) return;

        var scope = file_scope.getBody();
        if (scope == null) return;

        var nodes = scope.getNodes();
        if (nodes == null || nodes.size() == 0) return; // Skip empty files

        printf("  Generating docs for %s (%uint nodes)\n", file_meta.getAbsPath().data(), nodes.size());

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

        var out_file = std::string();
        out_file.append_view(mod_dir);
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

    func find_symbol(&mut self, name : &std::string_view) : *mut SymbolInfo {
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (sym.name.to_view().equals(name)) return sym;
        }
        return null;
    }

    func get_type_name(&self, type : *BaseType) : std::string_view {
        var kind = type.getKind();
        if (kind == BaseTypeKind.Bool) return std::string_view("bool");
        if (kind == BaseTypeKind.Double) return std::string_view("double");
        if (kind == BaseTypeKind.Float) return std::string_view("float");
        if (kind == BaseTypeKind.String) return std::string_view("string");
        if (kind == BaseTypeKind.Void) return std::string_view("void");
        if (kind == BaseTypeKind.IntN) {
            var intn = type as *IntNType;
            var int_kind = intn.get_intn_type_kind();
            if (int_kind == IntNTypeKind.I8) return std::string_view("i8");
            if (int_kind == IntNTypeKind.I16) return std::string_view("i16");
            if (int_kind == IntNTypeKind.I32) return std::string_view("i32");
            if (int_kind == IntNTypeKind.I64) return std::string_view("i64");
            if (int_kind == IntNTypeKind.U8) return std::string_view("u8");
            if (int_kind == IntNTypeKind.U16) return std::string_view("u16");
            if (int_kind == IntNTypeKind.U32) return std::string_view("u32");
            if (int_kind == IntNTypeKind.U64) return std::string_view("u64");
            if (int_kind == IntNTypeKind.Int) return std::string_view("int");
            if (int_kind == IntNTypeKind.UInt) return std::string_view("uint");
            if (int_kind == IntNTypeKind.Char) return std::string_view("char");
            if (int_kind == IntNTypeKind.UChar) return std::string_view("uchar");
            if (int_kind == IntNTypeKind.Short) return std::string_view("short");
            if (int_kind == IntNTypeKind.UShort) return std::string_view("ushort");
            if (int_kind == IntNTypeKind.Long) return std::string_view("long");
            if (int_kind == IntNTypeKind.ULong) return std::string_view("ulong");
            if (int_kind == IntNTypeKind.LongLong) return std::string_view("longlong");
            if (int_kind == IntNTypeKind.ULongLong) return std::string_view("ulonglong");
        }
        return std::string_view("unknown");
    }

    func render_type(&mut self, type : *BaseType, html : &mut std::string) {
        if (type == null) {
            html.append_view("void");
            return;
        }
        var kind = type.getKind();
        if (kind == BaseTypeKind.Pointer) {
            html.append_view("*");
            self.render_type((type as *PointerType).getChildType(), html);
        } else if (kind == BaseTypeKind.Reference) {
            html.append_view("&amp;");
            self.render_type((type as *ReferenceType).getChildType(), html);
        } else if (kind == BaseTypeKind.Linked) {
            var linked = type as *LinkedType;
            var node = linked.getLinkedNode();
            if (node != null) {
                var name = get_node_name(node);
                var sym = self.find_symbol(name);
                if (sym != null) {
                    html.append_view("<a href='");
                    // Simple absolute path for now; better to use relative paths later
                    html.append_view("/");
                    html.append_view(sym.mod_name.to_view());
                    html.append_view("/");
                    var f_id = std::string("");
                    f_id.append_uinteger(sym.file_id as ubigint);
                    html.append_view(f_id.to_view());
                    html.append_view(".html'>");
                    html.append_view(name);
                    html.append_view("</a>");
                } else {
                    html.append_view(name);
                }
            } else {
                html.append_view("unknown");
            }
        } else if (kind == BaseTypeKind.Generic) {
            var gen = type as *GenericType;
            var linked = gen.getLinkedType();
            if (linked != null) {
                var node = linked.getLinkedNode();
                if (node != null) {
                    var name = get_node_name(node);
                    html.append_view(name);
                }
            }
            var arg_count = gen.getArgumentCount();
            if (arg_count > 0) {
                html.append_view("&lt;");
                for (var i = 0u; i < arg_count; i++) {
                    if (i > 0) html.append_view(", ");
                    self.render_type(gen.getArgumentType(i), html);
                }
                html.append_view("&gt;");
            }
        } else {
            html.append_view(self.get_type_name(type));
        }
    }

    func document_node(&mut self, node : *ASTNode, html : &mut std::string, tokens : std::span<Token>) {
        var name = get_node_name(node);
        if (name.size() == 0) return;

        var kind = node.getKind();
        var kind_label = get_kind_label(kind);
        var access = node.getAccessSpecifier();

        html.append_view("<div class='node'>");
        
        // Access Specifier Badge
        if (access == AccessSpecifier.Public) {
            html.append_view("<span class='attr-badge'>Public</span>");
        } else if (access == AccessSpecifier.Internal) {
            html.append_view("<span class='attr-badge'>Internal</span>");
        } else if (access == AccessSpecifier.Private) {
            html.append_view("<span class='attr-badge'>Private</span>");
        }

        // Kind Badge
        html.append_view("<span class='kind-badge'>");
        html.append_view(kind_label);
        html.append_view("</span> ");

        // Node Title
        html.append_view("<span class='node-title'>");
        html.append_view(name);
        html.append_view("</span>");

        // Signature and Attributes
        html.append_view("<div class='signature'>");
        
        if (kind == ASTNodeKind.FunctionDecl) {
            var decl = node as *FunctionDeclaration;
            var attrs : FuncDeclAttributesCBI = zeroed<FuncDeclAttributesCBI>();
            decl.getAttributes(&mut attrs);
            
            if (attrs.is_comptime) html.append_view("<span class='attr-badge'>comptime</span> ");
            if (attrs.is_extern) html.append_view("<span class='attr-badge'>extern</span> ");
            
            html.append_view("func ");
            html.append_view(name);
            html.append_view("(");
            var params = decl.get_params();
            for (var i = 0u; i < params.size(); i++) {
                if (i > 0) html.append_view(", ");
                var param = params.get(i);
                html.append_view(param.getName());
                html.append_view(" : ");
                self.render_type(param.getType(), html);
            }
            html.append_view(") : ");
            self.render_type(decl.getReturnType(), html);
            
        } else if (kind == ASTNodeKind.InterfaceDecl) {
            var def = node as *InterfaceDefinition;
            var attrs = zeroed<InterfaceDefinitionAttrsCBI>();
            def.getAttributes(&mut attrs);
            if (attrs.is_static) html.append_view("<span class='attr-badge'>static</span> ");
            html.append_view("interface ");
            html.append_view(name);
        } else if (kind == ASTNodeKind.TypealiasStmt) {
            var stmt = node as *TypealiasStatement;
            var attrs = zeroed<TypealiasDeclAttributesCBI>();
            stmt.getAttributes(&mut attrs);
            if (attrs.is_comptime) html.append_view("<span class='attr-badge'>comptime</span> ");
            html.append_view("type ");
            html.append_view(name);
            html.append_view(" = ");
            self.render_type(stmt.getActualType(), html);
        } else {
            html.append_view(kind_label);
            html.append_view(" ");
            html.append_view(name);
        }
        
        html.append_view("</div>");

        // Decode source location for comments
        var encoded_loc = node.getEncodedLocation();
        var loc_data = self.ctx.decodeLocation(encoded_loc);
        var comment = find_comment_before(tokens, loc_data.lineStart);

        if (comment.size() > 0) {
            html.append_view("<div class='comment'>");
            html.append_view(self.clean_comment(comment).to_view());
            html.append_view("</div>");
        }

        // Definition Location
        html.append_view("<div style='font-size: 0.8rem; color: var(--text-muted); margin-top: 1rem;'>Defined at line ");
        var line_str = std::string("");
        line_str.append_uinteger(loc_data.lineStart as ubigint);
        html.append_view(line_str.to_view());
        html.append_view(", column ");
        var col_str = std::string("");
        col_str.append_uinteger(loc_data.charStart as ubigint);
        html.append_view(col_str.to_view());
        html.append_view("</div>");

        html.append_view("</div>");
    }

    func clean_comment(&self, comment : std::string_view) : std::string {
        // TODO: remove leading asterisks for block comments
        return std::string(comment.data(), comment.size());
    }

    func generate_index_html(&mut self) {
        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
        html.append_view("<title>API Documentation</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style></head><body>");
        html.append_view("<div class='header'>API Documentation</div>");
        html.append_view("<div class='container'>");
        html.append_view("<h1>Modules</h1><ul>");

        // Simple linear list of modules for now
        // TODO: build a tree for deeper hierarchy
        var last_mod = std::string("");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (!sym.mod_name.equals(last_mod)) {
                html.append_view("<li><a href='");
                html.append_view(sym.mod_name.to_view());
                html.append_view("/index.html'>");
                html.append_view(sym.mod_name.to_view());
                html.append_view("</a></li>");
                last_mod = sym.mod_name.copy();
            }
        }
        html.append_view("</ul></div>");
        html.append_view(self.get_js());
        html.append_view("</body></html>");

        var out_file = self.output_dir.copy();
        out_file.append_view("/index.html");
        fs::write_text_file(out_file.data(), html.data() as *u8, html.size());
    }

    func generate_search_index(&mut self) {
        var json = std::string("[");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (i > 0) json.append_view(",");
            json.append_view("{\"n\":\"");
            json.append_view(sym.name.to_view());
            json.append_view("\",\"m\":\"");
            json.append_view(sym.mod_name.to_view());
            json.append_view("\",\"f\":");
            var f_id = std::string("");
            f_id.append_uinteger(sym.file_id as ubigint);
            json.append_view(f_id.to_view());
            json.append_view("}");
        }
        json.append_view("]");

        var out_file = self.output_dir.copy();
        out_file.append_view("/search_index.json");
        fs::write_text_file(out_file.data(), json.data() as *u8, json.size());
    }

    func get_js(&self) : std::string_view {
        return """
            <script>
            function toggleTheme() {
                const doc = document.documentElement;
                const theme = doc.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
                doc.setAttribute('data-theme', theme);
                localStorage.setItem('theme', theme);
            }
            const savedTheme = localStorage.getItem('theme') || (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
            document.documentElement.setAttribute('data-theme', savedTheme);
            </script>
            <button onclick='toggleTheme()' class='theme-toggle'>🌓</button>
        """;
    }

    func get_css(&self) : std::string_view {
        return """
            :root[data-theme='light'] {
                --bg: #ffffff;
                --bg-card: #f9fafb;
                --border: #e5e7eb;
                --text: #111827;
                --text-muted: #4b5563;
                --accent: #2563eb;
                --code-bg: #f3f4f6;
            }
            :root[data-theme='dark'] {
                --bg: #030712;
                --bg-card: #0f172a;
                --border: #1e293b;
                --text: #f8fafc;
                --text-muted: #94a3b8;
                --accent: #38bdf8;
                --code-bg: #1e293b;
            }
            body { 
                font-family: 'Inter', system-ui, -apple-system, sans-serif; 
                background: var(--bg); 
                color: var(--text); 
                padding: 0; margin: 0; 
                line-height: 1.5;
                transition: background 0.3s, color 0.3s;
            }
            .header { 
                background: var(--bg-card); 
                padding: 1rem 2rem; 
                border-bottom: 1px solid var(--border); 
                font-weight: 700; 
                color: var(--accent);
                display: flex;
                justify-content: space-between;
                align-items: center;
                position: sticky; top: 0; z-index: 100;
                backdrop-filter: blur(8px);
                background: rgba(var(--bg-card-rgb), 0.8);
            }
            .container { padding: 2rem; max-width: 1000px; margin: 0 auto; }
            .node { 
                background: var(--bg-card); 
                border: 1px solid var(--border); 
                padding: 1.5rem; 
                margin-bottom: 2rem; 
                border-radius: 16px;
                box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
                transition: transform 0.2s;
            }
            .node:hover { transform: translateY(-2px); }
            .kind-badge { 
                display: inline-block; 
                background: rgba(56, 189, 248, 0.15); 
                color: var(--accent); 
                padding: 2px 12px; border-radius: 9999px; 
                font-size: 0.75rem; font-weight: 600; 
                text-transform: uppercase; letter-spacing: 0.05em;
                margin-bottom: 0.5rem;
            }
            .node-title { font-size: 1.5rem; font-weight: 800; color: var(--text); margin-top: 0.5rem; display: block; }
            .signature { 
                font-family: 'JetBrains Mono', 'Fira Code', monospace;
                background: var(--code-bg);
                padding: 0.75rem 1rem;
                border-radius: 8px;
                margin: 1rem 0;
                font-size: 0.9rem;
                overflow-x: auto;
                border: 1px solid var(--border);
            }
            .comment { 
                color: var(--text-muted); 
                font-size: 1rem; 
                line-height: 1.7; 
                white-space: pre-wrap; 
                margin-top: 1rem;
                border-top: 1px solid var(--border);
                padding-top: 1rem;
            }
            .theme-toggle {
                background: var(--bg-card);
                border: 1px solid var(--border);
                color: var(--text);
                padding: 8px 12px;
                border-radius: 8px;
                cursor: pointer;
                font-size: 1.2rem;
                transition: all 0.2s;
            }
            .theme-toggle:hover { background: var(--border); }
            a { color: var(--accent); text-decoration: none; }
            a:hover { text-decoration: underline; }
            .attr-badge {
                display: inline-block;
                border: 1px solid var(--accent);
                color: var(--accent);
                padding: 1px 6px;
                border-radius: 4px;
                font-size: 0.7rem;
                margin-right: 4px;
                font-weight: 600;
            }
        """;
    }
}

}
