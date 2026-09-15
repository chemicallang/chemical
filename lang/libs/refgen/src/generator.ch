
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
        // Skip comment tokens, whitespace, and attributes
        if (tok_type == ChemicalTokenType.SingleLineComment || tok_type == ChemicalTokenType.MultiLineComment) {
            comment_start = idx;
            if (idx == 0) break;
            idx--;
        } else if (tok_type == ChemicalTokenType.Whitespace || tok_type == ChemicalTokenType.NewLine || tok_type == ChemicalTokenType.Annotation) { 
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

    // Skip //, /*, /**, */ and leading *
    var i = 0u;
    while (i < len) {
        if (data[i] == '/' && i + 1 < len && data[i+1] == '/') {
            i += 2;
            while (i < len && (data[i] == ' ' || data[i] == '/')) i++;
            continue;
        }
        if (data[i] == '/' && i + 1 < len && data[i+1] == '*') {
            i += 2;
            if (i < len && data[i] == '*') i++;
            continue;
        }
        if (data[i] == '*' && i + 1 < len && data[i+1] == '/') {
            i += 2;
            continue;
        }
        // Leading * on new lines
        if (data[i] == '*' && (i == 0 || data[i-1] == '\n' || (i > 0 && data[i-1] == ' ' && (i == 1 || data[i-2] == '\n')))) {
            i++;
            if (i < len && data[i] == ' ') i++;
            continue;
        }

        s.append(data[i]);
        i++;
    }
    return s.trim().to_string();
}

func process_doc_comment(comment : std::string_view, html : &mut std::string) {
    var clean = clean_comment(comment);
    var lines = std::string_view(clean.data(), clean.size()).split('\n');

    var main_desc = std::string("");
    var params = std::vector<std::string>();
    var returns = std::string("");
    var sees = std::vector<std::string>();

    for (var i = 0u; i < lines.size(); i++) {
        var line = lines.get(i).trim();
        if (line.starts_with("@param")) {
            params.push_back(std::string(line.subview(6, line.size()).trim().data()));
        } else if (line.starts_with("@return")) {
            returns = std::string(line.subview(7, line.size()).trim().data());
        } else if (line.starts_with("@see")) {
            sees.push_back(std::string(line.subview(4, line.size()).trim().data()));
        } else {
            if (!main_desc.empty()) main_desc.append_view("\n");
            main_desc.append_view(&line);
        }
    }

    if (!main_desc.empty()) {
        html.append_view("<div class='doc-main'>");
        html.append_view(main_desc.to_view());
        html.append_view("</div>");
    }

    if (params.size() > 0) {
        html.append_view("<div class='doc-section'><h4>Parameters</h4><ul class='doc-list'>");
        for (var i = 0u; i < params.size(); i++) {
            html.append_view("<li><b>");
            var p = params.get_ptr(i).to_view();
            var space = p.find(" ");
            if (space != -1u) {
                html.append_view(p.subview(0, space));
                html.append_view("</b> ");
                html.append_view(p.subview(space + 1, p.size()));
            } else {
                html.append_view(&p);
                html.append_view("</b>");
            }
            html.append_view("</li>");
        }
        html.append_view("</ul></div>");
    }

    if (!returns.empty()) {
        html.append_view("<div class='doc-section'><h4>Returns</h4><p>");
        html.append_view(returns.to_view());
        html.append_view("</p></div>");
    }

    if (sees.size() > 0) {
        html.append_view("<div class='doc-section'><h4>See Also</h4><ul class='doc-list'>");
        for (var i = 0u; i < sees.size(); i++) {
            html.append_view("<li>");
            html.append_view(sees.get_ptr(i).to_view());
            html.append_view("</li>");
        }
        html.append_view("</ul></div>");
    }
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
    } else if (kind == ASTNodeKind.GenericFuncDecl) {
        return (node as *GenericFuncDecl).getMasterImpl().getName();
    } else if (kind == ASTNodeKind.GenericStructDecl) {
        return (node as *GenericStructDecl).getMasterImpl().getName();
    } else if (kind == ASTNodeKind.GenericVariantDecl) {
        return (node as *GenericVariantDecl).getMasterImpl().getName();
    } else if (kind == ASTNodeKind.GenericUnionDecl) {
        return (node as *GenericUnionDecl).getMasterImpl().getName();
    } else if (kind == ASTNodeKind.TypealiasStmt) {
        return (node as *TypealiasStatement).getName();
    } else if (kind == ASTNodeKind.GenericInterfaceDecl) {
        return (node as *GenericInterfaceDecl).getMasterImpl().getName();
    } else if (kind == ASTNodeKind.GenericTypeParam) {
        return (node as *GenericTypeParameter).getName();
    } else if (kind == ASTNodeKind.StructMember || kind == ASTNodeKind.VariantMember) {
        return (node as *BaseDefMember).getName();
    }
    return std::string_view();
}

// Get a kind label for documentation
func get_kind_label(kind : ASTNodeKind) : std::string_view {
    if (kind == ASTNodeKind.FunctionDecl || kind == ASTNodeKind.GenericFuncDecl) {
        return std::string_view("function");
    } else if (kind == ASTNodeKind.StructDecl || kind == ASTNodeKind.GenericStructDecl) {
        return std::string_view("struct");
    } else if (kind == ASTNodeKind.InterfaceDecl || kind == ASTNodeKind.GenericInterfaceDecl) {
        return std::string_view("interface");
    } else if (kind == ASTNodeKind.NamespaceDecl) {
        return std::string_view("namespace");
    } else if (kind == ASTNodeKind.EnumDecl) {
        return std::string_view("enum");
    } else if (kind == ASTNodeKind.VariantDecl || kind == ASTNodeKind.GenericVariantDecl) {
        return std::string_view("variant");
    } else if (kind == ASTNodeKind.UnionDecl || kind == ASTNodeKind.GenericUnionDecl) {
        return std::string_view("union");
    } else if (kind == ASTNodeKind.TypealiasStmt) {
        return std::string_view("typealias");
    } else if (kind == ASTNodeKind.StructMember || kind == ASTNodeKind.VariantMember) {
        return std::string_view("member");
    }
    return std::string_view("declaration");
}

func is_native_module(mod_name : std::string_view) : bool {
    // List of native modules based on d:\Programming\Cpp\zig-bootstrap\chemical\lang\libs
    var natives = "|atomic|compiler|core|crashsave|css_cbi|css_ide|css_parser|cstd|docgen|fs|html_cbi|html_comp|html_ide|html_parser|ide|js_cbi|json|lab|md|md_cbi|minlsp|net|page|preact_cbi|react_cbi|refgen|solid_cbi|std|test|test_env|transformer|";
    var search = std::string("|");
    search.append_view(&mod_name);
    search.append_view("|");
        var natives_str = std::string(natives);
        var natives_view = natives_str.to_view();
        return natives_view.contains(&search.to_view());
}

func is_alphanum(c : char) : bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

func highlight_chemical(code : std::string_view) : std::string {
    var kwds_view = std::string_view("func|var|const|struct|enum|namespace|public|private|if|else|while|for|return|break|continue|switch|case|default|import|using|as|in|true|false|null|defer|unsafe|impl|interface|union|bitfield|comptime|type|extend|trait|mut|self|Self|this|is|dyn|loop|new|destruct|dealloc|delete|provide|init|try|catch|throw|from|do|sizeof|alignof|protected|internal|any|void|alias|variant");
    var types_view = std::string_view("i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|bool|char|int|long|float|double|uint|ulong|short|ushort|uchar");

    var kwds = kwds_view.data()
    var types = types_view.data()

    var html = std::string("");
    var i = 0u;
    while (i < code.size()) {
        var c = code.data()[i];
        if (c == '<') {
            html.append_view("&lt;");
            i++;
        } else if (c == '>') {
            html.append_view("&gt;");
            i++;
        } else if (c == '&') {
            html.append_view("&amp;");
            i++;
        } else if (c == '"' || c == '\'') {
            var quote = c;
            var start = i;
            html.append_view("<span class='tok-str'>");
            html.append(c);
            i++;
            while (i < code.size() && code.data()[i] != quote) {
                if (code.data()[i] == '\\' && i + 1 < code.size()) {
                    html.append(code.data()[i]);
                    html.append(code.data()[i+1]);
                    i += 2;
                } else {
                    html.append(code.data()[i]);
                    i++;
                }
            }
            if (i < code.size()) {
                html.append(code.data()[i]);
                i++;
            }
            html.append_view("</span>");
        } else if (c == '/' && i + 1 < code.size() && (code.data()[i+1] == '/' || code.data()[i+1] == '*')) {
            html.append_view("<span class='tok-com'>");
            if (code.data()[i+1] == '/') {
                while (i < code.size() && code.data()[i] != '\n') {
                    html.append(code.data()[i]);
                    i++;
                }
            } else {
                html.append_view("/*");
                i += 2;
                while (i + 1 < code.size() && !(code.data()[i] == '*' && code.data()[i+1] == '/')) {
                    html.append(code.data()[i]);
                    i++;
                }
                if (i + 1 < code.size()) {
                    html.append_view("*/");
                    i += 2;
                }
            }
            html.append_view("</span>");
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            var start = i;
            while (i < code.size() && is_alphanum(code.data()[i])) i++;
            var word = std::string_view(code.data() + start, i - start);
            
            // Very simple keyword/type check (substring search in list for speed)
            // In a real implementation we'd use a set or split the string
            var span_class = std::string_view("");
            if (kwds_view.contains(&word)) span_class = std::string_view("tok-kwd");
            else if (types_view.contains(&word)) span_class = std::string_view("tok-type");
            
            if (!span_class.empty()) {
                html.append_view("<span class='");
                html.append_view(&span_class);
                html.append_view("'>");
                html.append_view(&word);
                html.append_view("</span>");
            } else {
                html.append_view(&word);
            }
        } else {
            html.append(c);
            i++;
        }
    }
    return html;
}

func get_relative_root(depth : uint) : std::string {
    if (depth == 0) return std::string(".");
    var s = std::string("");
    for (var i = 0u; i < depth; i++) {
        if (i > 0) s.append_view("/");
        s.append_view("..");
    }
    return s;
}

public struct SymbolInfo {
    var name : std::string
    var kind : ASTNodeKind
    var file_id : uint
    var filename : std::string
    var mod_name : std::string
    var parent_name : std::string
    var access : AccessSpecifier
    var encoded_loc : ubigint
}

public struct Generator {
    var output_dir : std::string
    var ctx : *TransformerContext
    var github_links : bool
    var git_ref : std::string
    var no_search : bool
    var base_url : std::string
    var index : std::vector<SymbolInfo>
    var sidebar_cache : std::unordered_map<std::string, std::string>
    var cur_rel_root : std::string

    func index_node_recursive(&mut self, node : *ASTNode, file_id : uint, mod_name : std::string_view, parent_name : std::string_view, filename : std::string_view) {
        var name = get_node_name(node);
        if (name.size() > 0) {
            self.index.push_back(SymbolInfo {
                name = std::string(name.data(), name.size()),
                kind = node.getKind(),
                file_id = file_id,
                filename = std::string(filename.data(), filename.size()),
                mod_name = std::string(mod_name.data(), mod_name.size()),
                parent_name = std::string(parent_name.data(), parent_name.size()),
                access = node.getAccessSpecifier(),
                encoded_loc = node.getEncodedLocation()
            });
        }

        var kind = node.getKind();
        if (kind == ASTNodeKind.NamespaceDecl) {
            var ns = node as *Namespace;
            var children = ns.get_body();
            if (children != null) {
                for (var i = 0u; i < children.size(); i++) {
                    self.index_node_recursive(children.get(i), file_id, mod_name, name, filename);
                }
            }
        } else if (kind == ASTNodeKind.StructDecl) {
            var def = node as *StructDefinition;
            var funcs = def.getFunctions();
            if (funcs != null) {
                for (var i = 0u; i < funcs.size(); i++) {
                    self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                }
            }
            var members = def.getMembers();
            if (members != null) {
                for (var i = 0u; i < members.size(); i++) {
                    self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                }
            }
        } else if (kind == ASTNodeKind.InterfaceDecl) {
            var def = node as *InterfaceDefinition;
            var funcs = def.getFunctions();
            if (funcs != null) {
                for (var i = 0u; i < funcs.size(); i++) {
                    self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                }
            }
        } else if (kind == ASTNodeKind.VariantDecl) {
            var def = node as *VariantDefinition;
            var members = def.getMembers();
            if (members != null) {
                for (var i = 0u; i < members.size(); i++) {
                    self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                }
            }
        } else if (kind == ASTNodeKind.UnionDecl) {
            var def = node as *UnionDef;
            var members = def.getMembers();
            if (members != null) {
                for (var i = 0u; i < members.size(); i++) {
                    self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                }
            }
            var funcs = def.getFunctions();
            if (funcs != null) {
                for (var i = 0u; i < funcs.size(); i++) {
                    self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                }
            }
        } else if (kind == ASTNodeKind.GenericStructDecl) {
            var gdef = node as *GenericStructDecl;
            var def = gdef.getMasterImpl();
            if (def != null) {
                var funcs = def.getFunctions();
                if (funcs != null) {
                    for (var i = 0u; i < funcs.size(); i++) {
                        self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                    }
                }
                var members = def.getMembers();
                if (members != null) {
                    for (var i = 0u; i < members.size(); i++) {
                        self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                    }
                }
            }
        } else if (kind == ASTNodeKind.GenericVariantDecl) {
            var gdef = node as *GenericVariantDecl;
            var def = gdef.getMasterImpl();
            if (def != null) {
                var members = def.getMembers();
                if (members != null) {
                    for (var i = 0u; i < members.size(); i++) {
                        self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                    }
                }
            }
        } else if (kind == ASTNodeKind.GenericUnionDecl) {
            var gdef = node as *GenericUnionDecl;
            var def = gdef.getMasterImpl();
            if (def != null) {
                var funcs = def.getFunctions();
                if (funcs != null) {
                    for (var i = 0u; i < funcs.size(); i++) {
                        self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                    }
                }
                var members = def.getMembers();
                if (members != null) {
                    for (var i = 0u; i < members.size(); i++) {
                        self.index_node_recursive(members.get(i), file_id, mod_name, name, filename);
                    }
                }
            }
        } else if (kind == ASTNodeKind.GenericInterfaceDecl) {
            var gdef = node as *GenericInterfaceDecl;
            var def = gdef.getMasterImpl();
            if (def != null) {
                var funcs = def.getFunctions();
                if (funcs != null) {
                    for (var i = 0u; i < funcs.size(); i++) {
                        self.index_node_recursive(funcs.get(i), file_id, mod_name, name, filename);
                    }
                }
            }
        }
    }    // Add more recursive containers if needed (variants, unions, etc.)

    public func index_module(&mut self, module : *TransformerModule) {
        var mod_name = module.getName();
        var count = module.getFileCount();
        for (var i = 0u; i < count; i++) {
            var file_meta = module.getFile(i);
            var file_id = file_meta.getFileId();
            var abs_path = file_meta.getAbsPath();
            var filename = std::string_view("");
            var last_slash = abs_path.find_last("/");
            if (last_slash == -1u) last_slash = abs_path.find_last("\\");
            if (last_slash != -1u) {
                filename = abs_path.subview(last_slash + 1, abs_path.size());
            } else {
                filename = abs_path;
            }

            var file_scope = file_meta.getFileScope();
            if (file_scope == null) continue;
            var scope = file_scope.getBody();
            if (scope == null) continue;
            var nodes = scope.getNodes();
            if (nodes == null) continue;
            
            for (var j = 0u; j < nodes.size(); j++) {
                self.index_node_recursive(nodes.get(j), file_id, mod_name, std::string_view(""), filename);
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
        mod_dir.append_view(&mod_name);
        
        res = fs::create_dir_all(mod_dir.data());
        if (res is std::Result.Err) {
            var Err(e) = res else unreachable;
            printf("Error creating module directory %s: %s\n", mod_dir.data(), e.message().data());
            return;
        }

        var mod_dir_view = mod_dir.to_view();
        var count = module.getFileCount();
        for (var i = 0u; i < count; i++) {
            var file_meta = module.getFile(i);
            self.generate_file_docs(file_meta, mod_dir_view, mod_name);
        }

        self.generate_module_index(mod_name, mod_dir_view);
    }

    func generate_module_index(&mut self, mod_name : std::string_view, mod_dir : std::string_view) {
        var rel_root = get_relative_root(1);
        self.cur_rel_root = rel_root.copy();

        // Gather stats for this module
        var sym_count : uint = 0;
        var pub_count : uint = 0;
        for (var si = 0u; si < self.index.size(); si++) {
            var ssym = self.index.get_ptr(si);
            if (ssym.mod_name.to_view().equals(&mod_name)) {
                sym_count++;
                if (ssym.access == AccessSpecifier.Public) pub_count++;
            }
        }
        var stats = std::string("");
        stats.append_view("<span class='stat-chip'><b>");
        stats.append_uinteger(sym_count as ubigint);
        stats.append_view("</b> symbols</span>");
        stats.append_view("<span class='stat-chip'><b>");
        stats.append_uinteger(pub_count as ubigint);
        stats.append_view("</b> public</span>");

        var crumb = std::string("<a class='crumb' href='../index.html'>Reference</a>");
        var html = self.page_head(mod_name, "Chemical API Documentation for module", "module", crumb.to_view(), mod_name, "", stats.to_view());

        var sb = self.generate_sidebar(rel_root.to_view());
        html.append_string(sb.copy());

        html.append_view("<div class='main-content'>");
        
        // Dependency Graph Visualization
        html.append_view("<div class='dependency-graph'>");
        html.append_view("<h3>Module Dependencies</h3>");
        html.append_view("<div class='mermaid'>");
        html.append_string(self.generate_dependency_graph(mod_name));
        html.append_view("</div></div>");

        html.append_view("<div class='symbols-header'>");
        html.append_view("<h3>Symbols</h3>");
        html.append_view("<div class='filter-box'><label class='switch'><input type='checkbox' id='public-only' onchange='applyFilter()'><span class='slider'></span></label> <span>Public only</span></div>");
        html.append_view("<button class='theme-btn' onclick='toggleExpandAll(true)'>Expand</button>");
        html.append_view("<button class='theme-btn' onclick='toggleExpandAll(false)'>Collapse</button>");
        html.append_view("</div><div class='symbol-tree'>");
        
        // Pass 1: Top-level symbols
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (sym.mod_name.to_view().equals(&mod_name) && sym.parent_name.empty()) {
                var is_public = sym.access == AccessSpecifier.Public;
                html.append_view("<div class='top-level-sym ");
                if (!is_public) html.append_view("non-public");
                html.append_view("'>");
                html.append_view("<div class='sym-header'>");
                html.append_view("<div style='display: flex; align-items: center; flex-wrap: wrap; gap: 0.5rem;'>");
                html.append_view("<a href='./");
                var f_id = std::string("");
                f_id.append_uinteger(sym.file_id as ubigint);
                html.append_view(f_id.to_view());
                html.append_view(".html#");
                html.append_view(sym.name.to_view());
                html.append_view("'><b>");
                html.append_view(sym.name.to_view());
                html.append_view("</a> <small>(");
                html.append_view(get_kind_label(sym.kind));
                html.append_view(")</small> ");
                html.append_view("<span class='sym-loc'>in <i>");
                html.append_view(sym.filename.to_view());
                html.append_view("</i></span> ");
                
                var has_children = false;
                for (var j = 0u; j < self.index.size(); j++) {
                    var child = self.index.get_ptr(j);
                    if (child.mod_name.to_view().equals(&mod_name) && child.parent_name.to_view().equals(&sym.name.to_view())) {
                        has_children = true;
                        break;
                    }
                }
                
                if (sym.access != AccessSpecifier.Public) {
                    html.append_view("<span class='attr-badge badge-private'>Private</span>");
                }
                html.append_view("</div>");
                
                if (has_children) {
                    html.append_view("<button class='collapse-btn' onclick='toggleCollapse(this)'>&#9654;</button>");
                }
                html.append_view("</div>");

                if (has_children) {
                    html.append_view("<div class='collapsible-content' style='display:none;'>");
                    html.append_view("<ul class='nav-list' style='margin-left: 1.5rem; margin-top: 0.5rem; border-left: 1px solid var(--border); padding-left: 1rem;'>");
                    for (var j = 0u; j < self.index.size(); j++) {
                        var child = self.index.get_ptr(j);
                        if (child.mod_name.to_view().equals(&mod_name) && child.parent_name.to_view().equals(&sym.name.to_view())) {
                            html.append_view("<li><a href='./");
                            var cf_id = std::string("");
                            cf_id.append_uinteger(child.file_id as ubigint);
                            html.append_view(cf_id.to_view());
                            html.append_view(".html#");
                            html.append_view(child.name.to_view());
                            html.append_view("'>");
                            html.append_view(child.name.to_view());
                            html.append_view("</a> <small>(");
                            html.append_view(get_kind_label(child.kind));
                            html.append_view(")</small></li>");
                        }
                    }
                    html.append_view("</ul></div>");
                }
                html.append_view("</div><hr style='border: 0; border-top: 1px solid var(--border); margin: 1rem 0;'>");
            }
        }
        
        html.append_view("</div>");
        html.append_view(self.footer_html());
        html.append_view("</div></div>");
        var js = self.get_js(rel_root.to_view());
        html.append_string(js.copy());
        html.append_view("</body></html>");

        var out_file = std::string(mod_dir.data(), mod_dir.size());
        out_file.append_view("/index.html");
        fs::write_text_file(out_file.data(), html.data() as *u8, html.size());
    }

    public func finish(&mut self) {
        self.generate_index_html();
        if (!self.no_search) {
            self.generate_search_index();
        }
        self.generate_sitemap();
    }

    func render_github_link(&self, abs_path : &std::string_view, line : uint, html : &mut std::string) {
        if (!self.github_links) return;
        var path_win = std::string(abs_path.data(), abs_path.size());
        var path = path_win.copy();
        for (var i = 0u; i < path.size(); i++) {
            if(path.get(i) == '\\') path.set(i, '/');
        }

        var src_idx = path.to_view().find("src/");
        if (src_idx != -1u) {
            var rel_path = path.to_view().subview(src_idx + 4, path.size());
            var libs_idx = path.to_view().find("libs/");
            if (libs_idx != -1u) {
                var after_libs = path.to_view().subview(libs_idx + 5, path.size());
                var slash_idx = after_libs.find("/");
                if (slash_idx != -1u) {
                    var m_name = after_libs.subview(0, slash_idx);
                    if (is_native_module(m_name)) {
                        html.append_view("<a class='git-link' href='https://github.com/chemicallang/chemical/tree/");
                        html.append_view(self.git_ref.to_view());
                        html.append_view("/lang/libs/");
                        html.append_view(&m_name);
                        html.append_view("/src/");
                        html.append_view(&rel_path);
                        html.append_view("#L");
                        var l_str = std::string("");
                        l_str.append_uinteger(line as ubigint);
                        html.append_view(l_str.to_view());
                        html.append_view("' target='_blank'>");
                        html.append_view("<svg class='git-icon' viewBox='0 0 16 16' width='16' height='16'><path fill='currentColor' d='M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z'></path></svg>");
                        html.append_view("</a>");
                    }
                }
            }
        }
    }

    func find_module(&self, name : std::string_view) : *TransformerModule {
        var deps = self.ctx.getFlattenedModules();
        for (var i = 0u; i < deps.size(); i++) {
            var m = deps.get(i) as *TransformerModule;
            if (m.getName().equals(&name)) return m;
        }
        return null;
    }

    func add_module_deps(&self, mod : *TransformerModule, mermaid : &mut std::string, visited : &mut std::vector<std::string>) {
        var mod_name = mod.getName();
        var d_count = mod.getDependencyCount();
        for (var i = 0u; i < d_count; i++) {
            var d = mod.getDependency(i);
            var d_name = d.getName();
            mermaid.append_view("    ");
            mermaid.append_view(&mod_name);
            mermaid.append_view(" --> ");
            mermaid.append_view(&d_name);
            mermaid.append_view("[");
            mermaid.append_view(&d_name);
            mermaid.append_view("]\n");
            // Check if already visited to avoid infinite recursion
            var already = false;
            for (var j = 0u; j < visited.size(); j++) {
                var visited_str = visited.get(j);
                var visited_view = visited_str.to_view();
                if (visited_view.equals(&d_name)) {
                    already = true;
                    break;
                }
            }
            if (!already) {
                visited.push_back(std::string(d_name.data(), d_name.size()));
                var child_mod = self.find_module(d_name);
                if (child_mod != null) {
                    self.add_module_deps(child_mod, mermaid, visited);
                }
            }
        }
    }

    func generate_dependency_graph(&mut self, mod_name : std::string_view) : std::string {
        var mod = self.find_module(mod_name);
        if (mod == null) return std::string("");
        
        var mermaid = std::string("graph LR\n");
        mermaid.append_view("    ");
        mermaid.append_view(&mod_name);
        mermaid.append_view("[");
        mermaid.append_view(&mod_name);
        mermaid.append_view("]\n");
        
        var visited = std::vector<std::string>();
        visited.push_back(std::string(mod_name.data(), mod_name.size()));
        self.add_module_deps(mod, &mut mermaid, &mut visited);
        return mermaid;
    }

    func generate_sidebar(&mut self, rel_root : &std::string_view) : &std::string {
        var key = std::string(rel_root.data(), rel_root.size());
        if (self.sidebar_cache.contains(&key)) {
            var cached = self.sidebar_cache.get_ptr(&key);
            return &*cached;
        }

        var html = std::string("<div class='sidebar'>");
        html.append_view("<div class='search-box'><input type='text' id='search-input' placeholder='Search...' oninput='searchSymbols()'><div id='search-results'></div></div>");
        
        html.append_view("<h3>Themes</h3><div class='theme-toggles'>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('dark')\">Dark</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('light')\">Light</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('playground')\">Playground</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('playground-light')\">Playground Light</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('paper')\">Paper</button>");
        html.append_view("</div>");
        

        html.append_view("<h3>Modules</h3><ul class='nav-list'>");
        var last_mod = std::string("");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (!sym.mod_name.equals(&last_mod)) {
                html.append_view("<li><a href='");
                html.append_view(rel_root);
                html.append_view("/");
                html.append_view(sym.mod_name.to_view());
                html.append_view("/index.html'>");
                html.append_view(sym.mod_name.to_view());
                html.append_view("</a></li>");
                last_mod = sym.mod_name.copy();
            }
        }
        html.append_view("</ul></div>");
        self.sidebar_cache.insert(key.copy(), html.copy());
        var stored = self.sidebar_cache.get_ptr(&key);
        return &*stored;
    }

    func generate_file_docs(&mut self, file_meta : *ASTFileMetaData, mod_dir : std::string_view, mod_name : std::string_view) {
        var file_scope = file_meta.getFileScope();
        if (file_scope == null) return;

        var scope = file_scope.getBody();
        if (scope == null) return;

        var nodes = scope.getNodes();
        if (nodes == null || nodes.size() == 0) return;

        var file_id = file_meta.getFileId();
        var tokens = self.ctx.getFileTokens(file_id);
        var rel_root = get_relative_root(1);
        self.cur_rel_root = rel_root.copy();
        var abs_path = file_meta.getAbsPath();

        // Extract filename from abs_path
        var filename = std::string_view("");
        var last_slash = abs_path.find_last("/");
        if (last_slash == -1u) last_slash = abs_path.find_last("\\");
        if (last_slash != -1u) {
            filename = abs_path.subview(last_slash + 1, abs_path.size());
        } else {
            filename = abs_path;
        }

        // Count top-level declarations for stats
        var decl_count : uint = 0;
        {
            var ci = 0u;
            while (ci < nodes.size()) {
                var cname = get_node_name(nodes.get(ci));
                if (cname.size() > 0) decl_count++;
                ci++;
            }
        }
        var stats = std::string("");
        stats.append_view("<span class='stat-chip'><b>");
        stats.append_uinteger(decl_count as ubigint);
        stats.append_view("</b> declarations</span>");
        stats.append_view("<span class='stat-chip'>");
        stats.append_view(&mod_name);
        stats.append_view("</span>");

        var title = std::string("");
        title.append_view(&filename);
        title.append_view(" - Chemical API");

        var sub = std::string("Declarations in <code>");
        sub.append_view(&filename);
        sub.append_view("</code>");
        var sub_view = sub.to_view();

        var html = self.page_head(title.to_view(), "Chemical API Documentation for", "api", "", "", filename, sub_view);

        var sb = self.generate_sidebar(rel_root.to_view());
        html.append_string(sb.copy());

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='symbols-header'><h3>Declarations</h3><div class='filter-box'><label class='switch'><input type='checkbox' id='public-only' onchange='applyFilter()'><span class='slider'></span></label> <span>Public only</span></div></div>");

        if (decl_count == 0) {
            html.append_view("<div class='empty-state'><span class='empty-icon'>~</span><p>No public declarations found in this file.</p></div>");
        }

        var i = 0u;
        while (i < nodes.size()) {
            var node = nodes.get(i);
            var rel_view = rel_root.to_view();
            self.document_node(node, &mut html, tokens, &rel_view, &abs_path);
            i++;
        }

        // Debug Info section
        html.append_view("<button class='debug-toggle' onclick='toggleDebug()'>Show Debug Info</button>");
        html.append_view("<div id='debug-info' style='display:none;'>Generated from: ");
        html.append_view(&abs_path);
        html.append_view("<br>File ID: ");
        var f_id_str = std::string("");
        f_id_str.append_uinteger(file_id as ubigint);
        html.append_view(f_id_str.to_view());
        html.append_view("</div>");

        html.append_view(self.footer_html());
        html.append_view("</div></div>");
        var js = self.get_js(rel_root.to_view());
        html.append_string(js.copy());
        html.append_view("</body></html>");

        var out_file = std::string(mod_dir.data(), mod_dir.size());
        out_file.append_view("/");
        out_file.append_view(f_id_str.to_view());
        out_file.append_view(".html");

        fs::write_text_file(out_file.data(), html.data() as *u8, html.size());
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
            if (int_kind == IntNTypeKind.Int128) return std::string_view("i128");
            if (int_kind == IntNTypeKind.UInt128) return std::string_view("u128");
            if (int_kind == IntNTypeKind.Char) return std::string_view("char");
            if (int_kind == IntNTypeKind.UChar) return std::string_view("uchar");
            if (int_kind == IntNTypeKind.Short) return std::string_view("short");
            if (int_kind == IntNTypeKind.UShort) return std::string_view("ushort");
            if (int_kind == IntNTypeKind.Int) return std::string_view("int");
            if (int_kind == IntNTypeKind.UInt) return std::string_view("uint");
            if (int_kind == IntNTypeKind.I32) return std::string_view("i32");
            if (int_kind == IntNTypeKind.U32) return std::string_view("u32");
            if (int_kind == IntNTypeKind.I64) return std::string_view("i64");
            if (int_kind == IntNTypeKind.U64) return std::string_view("u64");
        }
        if (kind == BaseTypeKind.Any) return std::string_view("any");
        return std::string_view("unknown");
    }

    func render_type(&mut self, type : *BaseType, html : &mut std::string, rel_root : &std::string_view) {
        if (type == null) {
            html.append_view("void");
            return;
        }
        var kind = type.getKind();
        if (kind == BaseTypeKind.Pointer) {
            html.append_view("*");
            self.render_type((type as *PointerType).getChildType(), html, rel_root);
        } else if (kind == BaseTypeKind.Reference) {
            html.append_view("&amp;");
            self.render_type((type as *ReferenceType).getChildType(), html, rel_root);
        } else if (kind == BaseTypeKind.Generic) {
            var gen = type as *GenericType;
            var linked = gen.getLinkedType();
            if (linked != null) {
                self.render_type(linked, html, rel_root)
                html.append_view("&lt;");
                for (var i = 0u; i < gen.getArgumentCount(); i++) {
                    if (i > 0) html.append_view(", ");
                    self.render_type(gen.getArgumentType(i), html, rel_root);
                }
                html.append_view("&gt;");
            }
        } else if (kind == BaseTypeKind.Array) {
            var arr = type as *ArrayType;
            self.render_type(arr.getElementType(), html, rel_root);
            html.append_view("[");
            if (arr.getArraySize() > 0) {
                var s = std::string("");
                s.append_uinteger(arr.getArraySize() as ubigint);
                html.append_view(s.to_view());
            }
            html.append_view("]");
        } else if (kind == BaseTypeKind.Dynamic) {
            html.append_view("dyn ");
            self.render_type((type as *DynamicType).getChildType(), html, rel_root);
        } else if (kind == BaseTypeKind.Literal) {
            self.render_type((type as *LiteralType).getChildType(), html, rel_root);
        } else if (kind == BaseTypeKind.Function) {
            var ft = type as *FunctionType;
            html.append_view("func(");
            var params = ft.get_params();
            if (params != null) {
                for (var i = 0u; i < params.size(); i++) {
                    if (i > 0) html.append_view(", ");
                    self.render_type(params.get(i).getType(), html, rel_root);
                }
            }
            html.append_view(") : ");
            self.render_type(ft.getReturnType(), html, rel_root);
        } else if (kind == BaseTypeKind.Linked) {
            var linked = type as *LinkedType;
            var node = linked.getLinkedNode();
            if (node != null) {
                var name = get_node_name(node);
                var sym = self.find_symbol(&name);
                if (sym != null) {
                    html.append_view("<a href='");
                    html.append_view(rel_root);
                    html.append_view("/");
                    html.append_view(sym.mod_name.to_view());
                    html.append_view("/");
                    var f_id = std::string("");
                    f_id.append_uinteger(sym.file_id as ubigint);
                    html.append_view(f_id.to_view());
                    html.append_view(".html#");
                    html.append_view(&name);
                    html.append_view("'>");
                    html.append_view(&name);
                    html.append_view("</a>");
                } else {
                    html.append_view(&name);
                }
            } else {
                html.append_view("unknown");
            }
        } else {
            html.append_view(self.get_type_name(type));
        }
    }

    func document_node(&mut self, node : *ASTNode, html : &mut std::string, tokens : std::span<Token>, rel_root : &std::string_view, abs_path : &std::string_view) {
        var name = get_node_name(node);
        if (name.size() == 0) return;

        var kind = node.getKind();
        var kind_label = get_kind_label(kind);
        var access = node.getAccessSpecifier();

        html.append_view("<div class='node ");
        if (access != AccessSpecifier.Public) html.append_view("non-public ");
        html.append_view("' id='");
        html.append_view(&name);
        html.append_view("'>");
        
        // Header
        html.append_view("<div class='node-header' style='flex-direction: column; align-items: flex-start;'>");
        
        html.append_view("<div style='display: flex; justify-content: space-between; align-items: baseline; width: 100%;'>");
        html.append_view("<span class='node-title'>");
        html.append_view(&name);
        html.append_view("</span>");
        html.append_view("<a class='node-anchor' href='#");
        html.append_view(&name);
        html.append_view("' title='Copy link'>#</a>");
        
        if(true){
            var encoded_loc = node.getEncodedLocation();
            var loc_data = self.ctx.decodeLocation(encoded_loc);
            self.render_github_link(abs_path, loc_data.lineStart + 1, html);
        }
        
        html.append_view("</div>");

        html.append_view("<div style='display: flex; gap: 0.5rem; flex-wrap: wrap; margin-top: 0.5rem; align-items: center;'>");

        if (access == AccessSpecifier.Public) html.append_view("<span class='attr-badge'>Public</span>");

        html.append_view("<span class='kind-badge' style='margin-right: 0;'>");
        html.append_view(&kind_label);
        html.append_view("</span>");

        if (kind == ASTNodeKind.FunctionDecl || kind == ASTNodeKind.GenericFuncDecl) {
            var header_decl : *FunctionDeclaration = null;
            if (kind == ASTNodeKind.FunctionDecl) {
                header_decl = node as *FunctionDeclaration;
            } else {
                header_decl = (node as *GenericFuncDecl).getMasterImpl();
            }
            if (header_decl != null) {
                var attrs : FuncDeclAttributesCBI = header_decl.getAttributes();

                if (header_decl.isExtensionFn()) {
                    html.append_view("<span class='extension-tag' style='margin-left: 0;'>Extension</span>");
                }

                if (attrs.is_constructor_fn) {
                    if (attrs.is_implicit) html.append_view("<span class='attr-badge badge-implicit'>Implicit constructor</span>");
                    else html.append_view("<span class='attr-badge badge-constructor'>Constructor</span>");
                }
                if (attrs.is_override) {
                    html.append_view("<span class='attr-badge badge-override'>Override</span>");
                }
            }
        }
        html.append_view("</div></div>");

        // Signature with highlighting
        html.append_view("<div class='signature'>");
        
        if (kind == ASTNodeKind.FunctionDecl || kind == ASTNodeKind.GenericFuncDecl) {
            var decl : *FunctionDeclaration = null;
            var gparams : *mut VecRef<GenericTypeParameter> = null;

            if (kind == ASTNodeKind.FunctionDecl) {
                decl = node as *FunctionDeclaration;
            } else {
                var gdecl = node as *GenericFuncDecl;
                decl = gdecl.getMasterImpl();
                gparams = gdecl.getGenericParams();
            }

            var attrs : FuncDeclAttributesCBI = decl.getAttributes();
            
            html.append_view("<span class='tok-kwd'>func</span> ");
            html.append_view(&name);
            
            if (gparams != null) {
                html.append_view("&lt;");
                for (var i = 0u; i < gparams.size(); i++) {
                    if (i > 0) html.append_view(", ");
                    var gp = gparams.get(i);
                    html.append_view(gp.getName());
                    var def_t = gp.getDefaultType();
                    if (def_t != null) {
                        html.append_view(" = ");
                        self.render_type(def_t, html, rel_root);
                    }
                }
                html.append_view("&gt;");
            }
            
            html.append_view("(");
            var params = decl.get_params();
            for (var i = 0u; i < params.size(); i++) {
                if (i > 0) html.append_view(", ");
                var param = params.get(i);
                html.append_view(param.getName());
                html.append_view(" : ");
                self.render_type(param.getType(), html, rel_root);
            }
            html.append_view(") : ");
            self.render_type(decl.getReturnType(), html, rel_root);
            html.append_view("<button class='copy-btn' onclick='copySignature(this)'>copy</button>");
        } else if (kind == ASTNodeKind.StructDecl || kind == ASTNodeKind.InterfaceDecl || kind == ASTNodeKind.VariantDecl || 
                   kind == ASTNodeKind.GenericStructDecl || kind == ASTNodeKind.GenericInterfaceDecl || kind == ASTNodeKind.GenericVariantDecl ||
                   kind == ASTNodeKind.GenericUnionDecl || kind == ASTNodeKind.UnionDecl) {
            
            var container : *VariablesContainer = null;
            var gparams : *mut VecRef<GenericTypeParameter> = null;
            
            if (kind == ASTNodeKind.StructDecl) {
                html.append_view("<span class='tok-kwd'>struct</span> ");
                container = node as *StructDefinition;
            } else if (kind == ASTNodeKind.GenericStructDecl) {
                html.append_view("<span class='tok-kwd'>struct</span> ");
                var gdecl = node as *GenericStructDecl;
                container = gdecl.getMasterImpl();
                gparams = gdecl.getGenericParams();
            } else if (kind == ASTNodeKind.InterfaceDecl) {
                html.append_view("<span class='tok-kwd'>interface</span> ");
                container = node as *InterfaceDefinition;
            } else if (kind == ASTNodeKind.GenericInterfaceDecl) {
                html.append_view("<span class='tok-kwd'>interface</span> ");
                var gdecl = node as *GenericInterfaceDecl;
                container = gdecl.getMasterImpl();
                gparams = gdecl.getGenericParams();
            } else if (kind == ASTNodeKind.VariantDecl) {
                html.append_view("<span class='tok-kwd'>variant</span> ");
                container = node as *VariantDefinition;
            } else if (kind == ASTNodeKind.GenericVariantDecl) {
                html.append_view("<span class='tok-kwd'>variant</span> ");
                var gdecl = node as *GenericVariantDecl;
                container = gdecl.getMasterImpl();
                gparams = gdecl.getGenericParams();
            } else if (kind == ASTNodeKind.UnionDecl) {
                html.append_view("<span class='tok-kwd'>union</span> ");
                container = node as *UnionDef;
            } else if (kind == ASTNodeKind.GenericUnionDecl) {
                html.append_view("<span class='tok-kwd'>union</span> ");
                var gdecl = node as *GenericUnionDecl;
                container = gdecl.getMasterImpl();
                gparams = gdecl.getGenericParams();
            }
            
            html.append_view(&name);
            
            if (gparams != null) {
                html.append_view("&lt;");
                for (var i = 0u; i < gparams.size(); i++) {
                    if (i > 0) html.append_view(", ");
                    var gp = gparams.get(i);
                    html.append_view(gp.getName());
                    var def_t = gp.getDefaultType();
                    if (def_t != null) {
                        html.append_view(" = ");
                        self.render_type(def_t, html, rel_root);
                    }
                }
                html.append_view("&gt;");
            }
            
            var inherited_count = container.getInheritedCount();
            if (inherited_count > 0) {
                html.append_view(" : ");
                for (var i = 0u; i < inherited_count; i++) {
                    if (i > 0) html.append_view(", ");
                    self.render_type(container.getInheritedType(i), html, rel_root);
                }
            }
            
            var members : *mut VecRef<BaseDefMember> = null;
            if (kind == ASTNodeKind.StructDecl) {
                members = (node as *StructDefinition).getMembers();
            } else if (kind == ASTNodeKind.GenericStructDecl) {
                var d = (node as *GenericStructDecl).getMasterImpl();
                if (d != null) members = d.getMembers();
            } else if (kind == ASTNodeKind.VariantDecl) {
                members = (node as *VariantDefinition).getMembers();
            } else if (kind == ASTNodeKind.GenericVariantDecl) {
                var d = (node as *GenericVariantDecl).getMasterImpl();
                if (d != null) members = d.getMembers();
            }
            
            if (members != null && members.size() > 0) {
                html.append_view(" {\n");
                for (var i = 0u; i < members.size(); i++) {
                    var m = members.get(i);
                    html.append_view("    <span class='tok-kwd'>var</span> ");
                    html.append_view(m.getName());
                    
                    var mt = m.getType();
                    if (mt != null) {
                        html.append_view(" : ");
                        self.render_type(mt, html, rel_root);
                    }
                    html.append_view("\n");
                }
                html.append_view("}");
            }
        } else if (kind == ASTNodeKind.EnumDecl) {
            html.append_view("<span class='tok-kwd'>enum</span> ");
            html.append_view(&name);
            html.append_view(" {");
            var enum_decl = node as *EnumDeclaration;
            var members = enum_decl.getMembers();
            if (members != null && members.size() > 0) {
                html.append_view("\n    ");
                for (var i = 0u; i < members.size(); i++) {
                    if (i > 0) html.append_view(",\n    ");
                    html.append_view(members.get(i).getName());
                }
                html.append_view("\n");
            }
            html.append_view("}");
        } else if (kind == ASTNodeKind.UnionDecl) {
            html.append_view("<span class='tok-kwd'>union</span> ");
            html.append_view(&name);
            html.append_view(" {");
            var union_decl = node as *UnionDef;
            var members = union_decl.getMembers();
            if (members != null && members.size() > 0) {
                html.append_view("\n    ");
                for (var i = 0u; i < members.size(); i++) {
                    if (i > 0) html.append_view("\n    ");
                    var member = members.get(i);
                    html.append_view(member.getName());
                    html.append_view(" : ");
                    self.render_type(member.getType(), html, rel_root);
                }
                html.append_view("\n");
            }
            html.append_view("}");
        } else if (kind == ASTNodeKind.NamespaceDecl) {
            html.append_view("<span class='tok-kwd'>namespace</span> ");
            html.append_view(&name);
        } else if (kind == ASTNodeKind.TypealiasStmt) {
            var stmt = node as *TypealiasStatement;
            html.append_view("<span class='tok-kwd'>type</span> ");
            html.append_view(&name);
            html.append_view(" = ");
            self.render_type(stmt.getActualType(), html, rel_root);
        } else if (kind == ASTNodeKind.StructMember || kind == ASTNodeKind.VariantMember) {
            var member = node as *BaseDefMember;
            html.append_view("<span class='tok-kwd'>var</span> ");
            html.append_view(&name);
            var t = member.getType();
            if (t != null) {
                html.append_view(" : ");
                self.render_type(t, html, rel_root);
            }
        }
        html.append_view("</div>");
 
        // Comment
        if(true) {
            var encoded_loc = node.getEncodedLocation();
            var loc_data = self.ctx.decodeLocation(encoded_loc);
            var comment = find_comment_before(tokens, loc_data.lineStart);
            if (comment.size() > 0) {
                html.append_view("<div class='comment'>");
                process_doc_comment(comment, html);
                html.append_view("</div>");
            }
        }
 
        // Recursion for members
        if (kind == ASTNodeKind.NamespaceDecl) {
            var ns = node as *Namespace;
            var children = ns.get_body();
            if (children != null && children.size() > 0) {
                html.append_view("<div class='nested-container'>");
                for (var i = 0u; i < children.size(); i++) {
                    self.document_node(children.get(i), html, tokens, rel_root, abs_path);
                }
                html.append_view("</div>");
            }
        } else if (kind == ASTNodeKind.StructDecl || kind == ASTNodeKind.InterfaceDecl || kind == ASTNodeKind.VariantDecl || kind == ASTNodeKind.UnionDecl ||
                   kind == ASTNodeKind.GenericStructDecl || kind == ASTNodeKind.GenericInterfaceDecl || kind == ASTNodeKind.GenericVariantDecl || kind == ASTNodeKind.GenericUnionDecl) {
            
            var funcs : *mut VecRef<ASTNode> = null;
            var members : *mut VecRef<BaseDefMember> = null;
            
            if (kind == ASTNodeKind.StructDecl) {
                var def = node as *StructDefinition;
                funcs = def.getFunctions();
                members = def.getMembers();
            } else if (kind == ASTNodeKind.InterfaceDecl) {
                funcs = (node as *InterfaceDefinition).getFunctions();
            } else if (kind == ASTNodeKind.VariantDecl) {
                var def = node as *VariantDefinition;
                funcs = def.getFunctions();
                members = def.getMembers();
            } else if (kind == ASTNodeKind.UnionDecl) {
                var def = node as *UnionDef;
                funcs = def.getFunctions();
                members = def.getMembers();
            } else if (kind == ASTNodeKind.GenericStructDecl) {
                var def = (node as *GenericStructDecl).getMasterImpl();
                if (def != null) {
                    funcs = def.getFunctions();
                    members = def.getMembers();
                }
            } else if (kind == ASTNodeKind.GenericInterfaceDecl) {
                var def = (node as *GenericInterfaceDecl).getMasterImpl();
                if (def != null) funcs = def.getFunctions();
            } else if (kind == ASTNodeKind.GenericVariantDecl) {
                var def = (node as *GenericVariantDecl).getMasterImpl();
                if (def != null) {
                    funcs = def.getFunctions();
                    members = def.getMembers();
                }
            } else if (kind == ASTNodeKind.GenericUnionDecl) {
                var def = (node as *GenericUnionDecl).getMasterImpl();
                if (def != null) {
                    funcs = def.getFunctions();
                    members = def.getMembers();
                }
            }
            
            if ((funcs != null && funcs.size() > 0)) {
                html.append_view("<div class='nested-container'>");
                if (funcs != null) {
                    for (var i = 0u; i < funcs.size(); i++) {
                        self.document_node(funcs.get(i), html, tokens, rel_root, abs_path);
                    }
                }
                html.append_view("</div>");
            }
        }

        html.append_view("</div>");
    }

    func clean_comment(&self, comment : std::string_view) : std::string {
        // This is now redundant as we have it at namespace level, but keep it for compat or move it
        return clean_comment(comment);
    }

    func generate_index_html(&mut self) {
        var rel_root = std::string(".");
        self.cur_rel_root = rel_root.copy();

        // Unique module names in index order (index is built module-by-module)
        var mod_names = std::vector<std::string>();
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (mod_names.size() == 0) {
                mod_names.push_back(sym.mod_name.copy());
            } else {
                var last_name = mod_names.get_ptr(mod_names.size() - 1);
                if (!sym.mod_name.equals(&*last_name)) {
                    mod_names.push_back(sym.mod_name.copy());
                }
            }
        }

        var html = self.page_head("Chemical API Reference", "Chemical Language Core API Reference", "reference", "", "Chemical API Reference", "Documentation for the Chemical Language core libraries and standard modules.", "");

        var sb = self.generate_sidebar(rel_root.to_view());
        html.append_string(sb.copy());

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='module-grid'>");
        for (var m = 0u; m < mod_names.size(); m++) {
            var mname = mod_names.get_ptr(m);
            var mname_view = mname.to_view();
            html.append_view("<div class='module-card'>");
            html.append_view("<span class='card-arrow'>&#8594;</span>");
            html.append_view("<div class='kind-badge'>module</div>");
            html.append_view("<br><br><a href='./");
            html.append_view(&mname_view);
            html.append_view("/index.html'>");
            html.append_view(&mname_view);
            html.append_view("</a>");
            html.append_view("<span class='card-count'>");
            var cnt : uint = 0;
            for (var j = 0u; j < self.index.size(); j++) {
                if (self.index.get_ptr(j).mod_name.equals(&*mname)) cnt++;
            }
            html.append_uinteger(cnt as ubigint);
            html.append_view(" symbols</span>");
            html.append_view("</div>");
        }
        html.append_view("</div>");
        html.append_view(self.footer_html());
        html.append_view("</div></div>");
        var js = self.get_js(rel_root.to_view());
        html.append_string(js.copy());
        html.append_view("</body></html>");

        var out_file = self.output_dir.copy();
        out_file.append_view("/index.html");
        fs::write_text_file(out_file.data(), html.data() as *u8, html.size());
    }

    func generate_search_index(&mut self) {
        var js = std::string("window.searchIndex = [");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (i > 0) js.append_view(",");
            js.append_view("{\"n\":\"");
            js.append_view(sym.name.to_view());
            js.append_view("\",\"m\":\"");
            js.append_view(sym.mod_name.to_view());
            js.append_view("\",\"f\":");
            var f_id = std::string("");
            f_id.append_uinteger(sym.file_id as ubigint);
            js.append_view(f_id.to_view());
            // Store relative path to root for each symbol? No, let JS handle it using the current page's rel_root
            js.append_view("}");
        }
        js.append_view("];");

        var out_file = self.output_dir.copy();
        out_file.append_view("/search_index.js");
        fs::write_text_file(out_file.data(), js.data() as *u8, js.size());
    }

    func generate_sitemap(&mut self) {
        var xml = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        xml.append_view("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

        var added_files = std::unordered_map<std::string, bool>();

        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            
            var f_id_str = std::string("");
            f_id_str.append_uinteger(sym.file_id as ubigint);
            
            var rel_path = std::string(sym.mod_name.to_view());
            rel_path.append_view("/");
            rel_path.append_view(f_id_str.to_view());
            rel_path.append_view(".html");

            var b_url = self.base_url.to_view();
            var has_slash = false;
            if (b_url.size() > 0 && b_url.data()[b_url.size() - 1] as char == '/') has_slash = true;

            if (!added_files.contains(rel_path.copy())) {
                added_files.insert(rel_path.copy(), true);
                xml.append_view("  <url>\n    <loc>");
                xml.append_view(self.base_url.to_view());
                if (!has_slash) xml.append_view("/");
                xml.append_view(rel_path.to_view());
                xml.append_view("</loc>\n  </url>\n");
            }
            
            var mod_path = std::string(sym.mod_name.to_view());
            mod_path.append_view("/index.html");
            if (!added_files.contains(mod_path.copy())) {
                added_files.insert(mod_path.copy(), true);
                xml.append_view("  <url>\n    <loc>");
                xml.append_view(self.base_url.to_view());
                if (!has_slash) xml.append_view("/");
                xml.append_view(mod_path.to_view());
                xml.append_view("</loc>\n  </url>\n");
            }
        }
        
        xml.append_view("  <url>\n    <loc>");
        xml.append_view(self.base_url.to_view());
        var b_url = self.base_url.to_view();
        if (b_url.size() > 0 && b_url.data()[b_url.size() - 1] as char != '/') xml.append_view("/");
        xml.append_view("index.html</loc>\n  </url>\n");
        xml.append_view("</urlset>");

        var out_file = self.output_dir.copy();
        out_file.append_view("/sitemap.xml");
        fs::write_text_file(out_file.data(), xml.data() as *u8, xml.size());
    }

    // Shared page shell: head, fonts, CSS, theme init, topbar.
    // chip/crumb/title/sub/stats are raw HTML fragments (may be empty).
    func page_head(&self, title : std::string_view, desc : std::string_view, chip : std::string_view, crumb_html : std::string_view, title_html : std::string_view, sub_html : std::string_view, stats_html : std::string_view) : std::string {
        var html = std::string("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
        if (desc.size() > 0) {
            html.append_view("<meta name='description' content='");
            html.append_view(&desc);
            html.append_view("'>");
        }
        html.append_view("<title>");
        html.append_view(&title);
        html.append_view("</title>");
        html.append_view("<link rel='preconnect' href='https://fonts.googleapis.com'><link rel='preconnect' href='https://fonts.gstatic.com' crossorigin><link href='https://fonts.googleapis.com/css2?family=Sora:wght@400;500;600;700&family=IBM+Plex+Mono:wght@400;500;600&family=Outfit:wght@400;500;600;700;800&display=swap' rel='stylesheet'>");
        html.append_view("<style>");
        html.append_view(self.get_css());
        html.append_view("</style>");
        html.append_view("<script>function setTheme(t){document.documentElement.setAttribute('data-theme',t);localStorage.setItem('refgen-theme',t);if(window.__chemOnTheme)window.__chemOnTheme(t);}</script>");
        html.append_view("<script>try{var t=localStorage.getItem('refgen-theme')||'dark';document.documentElement.setAttribute('data-theme',t);}catch(e){document.documentElement.setAttribute('data-theme','dark');}</script>");
        html.append_view("</head><body>");
        html.append_view("<div class='topbar'>");
        html.append_view("<a class='brand' href='");
        var topbar_root = self.rel_root_for_topbar();
        var topbar_view = topbar_root.to_view();
        html.append_view(&topbar_view);
        html.append_view("/index.html'>Chemical</a>");
        if (crumb_html.size() > 0) {
            html.append_view("<span class='sep'>/</span>");
            html.append_view(&crumb_html);
        }
        html.append_view("<div class='topbar-spacer'></div>");
        html.append_view("<select id='theme-select' class='theme-select' aria-label='Theme' onchange='setTheme(this.value)'><option value='dark'>Dark</option><option value='light'>Light</option><option value='paper'>Paper</option><option value='playground'>Playground</option><option value='playground-light'>Playground Light</option></select>");
        html.append_view("</div>");
        html.append_view("<div class='layout'>");
        // page head block (inside main-content, appended by caller)
        var head = std::string("<div class='page-head'>");
        if (chip.size() > 0) {
            head.append_view("<span class='page-chip'>");
            head.append_view(&chip);
            head.append_view("</span>");
        }
        head.append_view("<h1>");
        head.append_view(&title_html);
        head.append_view("</h1>");
        if (sub_html.size() > 0) {
            head.append_view("<p class='page-sub'>");
            head.append_view(&sub_html);
            head.append_view("</p>");
        }
        if (stats_html.size() > 0) {
            head.append_view("<div class='stat-row'>");
            head.append_view(&stats_html);
            head.append_view("</div>");
        }
        head.append_view("</div>");
        html.append_string(head.copy());
        return html;
    }

    func rel_root_for_topbar(&self) : std::string {
        return self.cur_rel_root.copy();
    }

    func footer_html(&self) : std::string_view {
        return std::string_view("<div class='doc-footer'><span>Chemical API Reference</span><span><a href='https://chemicallang.com' target='_blank'>chemicallang.com</a> &nbsp;&middot;&nbsp; <a href='https://github.com/chemicallang/chemical' target='_blank'>GitHub</a></span></div><button id='back-top' aria-label='Back to top' onclick='window.scrollTo({top:0,behavior:'smooth'})'>&#8593;</button>");
    }

    func get_js(&self, rel_root : &std::string_view) : std::string {
        var s = std::string("<script src='");
        s.append_view(rel_root);
        s.append_view("/search_index.js'></script>");
        s.append_view("""
            <script>
            function toggleDebug() {
                const debug = document.getElementById('debug-info');
                debug.style.display = debug.style.display === 'none' ? 'block' : 'none';
            }

            function applyFilter() {
                const publicOnly = document.getElementById('public-only').checked;
                const nodes = document.querySelectorAll('.node, .top-level-sym');
                nodes.forEach(n => {
                    if (publicOnly && n.classList.contains('non-public')) {
                        n.style.display = 'none';
                    } else {
                        n.style.display = '';
                    }
                });
                
                const containers = document.querySelectorAll('.nested-container');
                containers.forEach(c => {
                    let hasVisible = false;
                    for (let i = 0; i < c.children.length; i++) {
                        if (c.children[i].style.display !== 'none') {
                            hasVisible = true;
                            break;
                        }
                    }
                    c.style.display = hasVisible ? '' : 'none';
                });
                localStorage.setItem('refgen-public-only', publicOnly);
            }
            
            // Restore filter
            const savedPublic = localStorage.getItem('refgen-public-only') === 'true';
            if (document.getElementById('public-only')) {
                document.getElementById('public-only').checked = savedPublic;
                applyFilter();
            }

            function searchSymbols() {
                const query = document.getElementById('search-input').value.toLowerCase();
                const results = document.getElementById('search-results');
                if (!query) {
                    results.style.display = 'none';
                    return;
                }
                if (!window.searchIndex) return;
                const matches = window.searchIndex.filter(s => s.n.toLowerCase().includes(query)).slice(0, 10);
                results.innerHTML = matches.map(m => `<div><a href='${REL_ROOT_VAR}/${m.m}/${m.f}.html#${m.n}'>${m.n} <small>(${m.m})</small></a></div>`).join('');
                results.style.display = 'block';
            }
            
            function toggleCollapse(btn) {
                const sym = btn.closest('.top-level-sym');
                if (!sym) return;
                const content = sym.querySelector('.collapsible-content');
                if (content && content.style.display === 'none') {
                    content.style.display = 'block';
                    btn.classList.add('expanded');
                } else if (content) {
                    content.style.display = 'none';
                    btn.classList.remove('expanded');
                }
            }

            function toggleExpandAll(expand) {
                const contents = document.querySelectorAll('.collapsible-content');
                const btns = document.querySelectorAll('.collapse-btn');
                contents.forEach(c => c.style.display = expand ? 'block' : 'none');
                btns.forEach(b => {
                    if (expand) b.classList.add('expanded');
                    else b.classList.remove('expanded');
                });
            }

            // Close search on outside click
            document.addEventListener('click', (e) => {
                const box = document.querySelector('.search-box');
                if (box && !box.contains(e.target)) {
                    document.getElementById('search-results').style.display = 'none';
                }
            });

            // Highlight the active module in the sidebar
            (function() {
                const path = location.pathname.replace(/\\/g, '/');
                const parts = path.split('/').filter(Boolean);
                const modSeg = parts.length >= 2 ? parts[parts.length - 2] : null;
                document.querySelectorAll('.nav-list a').forEach(a => {
                    if (modSeg && a.href.includes('/' + modSeg + '/index.html')) a.classList.add('active');
                });
            })();

            // Copy signature button
            function copySignature(btn) {
                const sig = btn.closest('.signature');
                if (!sig) return;
                navigator.clipboard.writeText(sig.textContent.replace(/^\\s*copy\\s*$/, '')).then(() => {
                    btn.textContent = 'copied';
                    btn.classList.add('copied');
                    setTimeout(() => { btn.textContent = 'copy'; btn.classList.remove('copied'); }, 1400);
                });
            }

            // Permalink anchors on node titles
            document.querySelectorAll('.node-anchor').forEach(a => {
                a.addEventListener('click', (e) => {
                    e.preventDefault();
                    const url = location.pathname + '#'; + a.getAttribute('href').slice(1);
                    navigator.clipboard.writeText(location.origin ? location.origin + url : url);
                    history.pushState(null, '', '#' + a.getAttribute('href').slice(1));
                });
            });

            // Back to top
            const backBtn = document.getElementById('back-top');
            if (backBtn) {
                window.addEventListener('scroll', () => {
                    backBtn.classList.toggle('visible', window.scrollY > 600);
                }, { passive: true });
            }

            // Theme sync for select + mermaid re-render hook
            const sel = document.getElementById('theme-select');
            if (sel) {
                try { sel.value = document.documentElement.getAttribute('data-theme') || 'dark'; } catch (e) {}
            }
            window.__chemOnTheme = function() {
                if (window.__chemRerenderMermaid) window.__chemRerenderMermaid();
            };
            </script>
            <script type="module">
            import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs';

            function palette() {
                const cs = getComputedStyle(document.documentElement);
                return {
                    bgCard: cs.getPropertyValue('--bg-card').trim() || '#101014',
                    text: cs.getPropertyValue('--text').trim() || '#E8E8EC',
                    accent: cs.getPropertyValue('--accent').trim() || '#4DA3FF',
                    codeBg: cs.getPropertyValue('--code-bg').trim() || '#060608',
                    border: cs.getPropertyValue('--border-strong').trim() || '#2A2A32'
                };
            }

            async function renderMermaid() {
                const els = document.querySelectorAll('.mermaid:not([data-rendered])');
                if (els.length === 0) return;
                const p = palette();
                mermaid.initialize({
                    startOnLoad: false,
                    theme: 'base',
                    securityLevel: 'loose',
                    themeVariables: {
                        primaryColor: p.bgCard,
                        primaryTextColor: p.text,
                        primaryBorderColor: p.border,
                        lineColor: p.accent,
                        secondaryColor: p.codeBg,
                        tertiaryColor: p.bgCard,
                        mainBkg: p.bgCard,
                        nodeBorder: p.border,
                        clusterBkg: p.codeBg,
                        clusterBorder: p.border,
                        edgeLabelBackground: p.bgCard,
                        fontSize: '14px'
                    },
                    flowchart: { curve: 'basis', htmlLabels: true }
                });
                for (const el of els) {
                    try {
                        const out = await mermaid.render('mmd-' + Math.random().toString(36).slice(2), el.textContent);
                        el.innerHTML = out.svg;
                        el.setAttribute('data-rendered', '1');
                    } catch (err) {
                        el.setAttribute('data-rendered', '1');
                        el.textContent = 'Diagram unavailable';
                    }
                }
            }

            renderMermaid();

            // Re-render diagrams with the active theme palette
            window.__chemRerenderMermaid = async function() {
                const els = document.querySelectorAll('.mermaid[data-rendered]');
                if (els.length === 0) return;
                els.forEach(el => { el.removeAttribute('data-rendered'); el.innerHTML = el.dataset.src || el.textContent; });
                renderMermaid();
            };
            </script>
        """);
        // Replace REL_ROOT_VAR with actual rel_root
        var res = std::string("");
        var raw = s.to_view();
        var placeholder = std::string_view("${REL_ROOT_VAR}");
        var last_idx = 0u;
        while (true) {
            var sub = raw.subview(last_idx, raw.size());
            var p_idx = sub.find(&placeholder);
            if (p_idx == -1u) {
                res.append_view(&sub);
                break;
            }
            res.append_view(sub.subview(0, p_idx));
            res.append_view(rel_root);
            last_idx = last_idx + p_idx + placeholder.size();
        }
        return res;
    }

    func get_css(&self) : std::string_view {
        return """
            /* ============================================================
               REFGEN — API REFERENCE DESIGN SYSTEM
               Precision instrument: near-black ink, hairline 1px lines,
               Sora display type, IBM Plex Mono code, signal blue accent.
               Themes: dark / light / paper / playground / playground-light
               ============================================================ */
            :root {
                --transition: 0.18s cubic-bezier(0.22, 1, 0.36, 1);
                --shadow: 0 1px 2px rgba(0, 0, 0, 0.25);
                --shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.35);
                --radius: 6px;
                --font-body: 'Outfit', 'Inter', system-ui, sans-serif;
                --font-code: 'IBM Plex Mono', 'JetBrains Mono', Consolas, monospace;
            }
            * { box-sizing: border-box; scroll-behavior: smooth; }
            :root[data-theme='light'] {
                --bg: #f8fafc; --bg-card: #ffffff; --border: #e2e8f0; --border-strong: #cbd5e1; --text: #0f172a; --text-muted: #64748b; --accent: #1d5fbf; --accent-ink: #ffffff; --accent-dim: rgba(29, 95, 191, 0.08); --code-bg: #f1f5f9; --btn-bg: #ffffff; --btn-text: #0f172a; --nested-bg: rgba(0,0,0,0.02);
            }
            :root[data-theme='dark'] {
                --bg: #0b0f19; --bg-card: #111827; --border: #1f2937; --border-strong: #374151; --text: #f3f4f6; --text-muted: #9ca3af; --accent: #60a5fa; --accent-ink: #0b0f19; --accent-dim: rgba(96, 165, 250, 0.10); --code-bg: #1f2937; --btn-bg: #1f2937; --btn-text: #f3f4f6; --nested-bg: rgba(255,255,255,0.03);
            }
            :root[data-theme='paper'] {
                --bg: #f4f1ea; --bg-card: #fdfcf9; --border: #e2ddd3; --border-strong: #cfc8ba; --text: #433f38; --text-muted: #7c7467; --accent: #8b5e34; --accent-ink: #fdfcf9; --accent-dim: rgba(139, 94, 52, 0.08); --code-bg: #e9e4d9; --btn-bg: #fdfcf9; --btn-text: #433f38; --nested-bg: rgba(0,0,0,0.03);
            }
            :root[data-theme='playground'] {
                --bg: #0A0A0C; --bg-card: #101014; --border: #1D1D23; --border-strong: #2A2A32; --text: #E8E8EC; --text-muted: #63636E; --accent: #4DA3FF; --accent-ink: #0A0A0C; --accent-dim: rgba(77, 163, 255, 0.12); --code-bg: #060608; --btn-bg: #17171D; --btn-text: #E8E8EC; --nested-bg: rgba(77, 163, 255, 0.05);
                --font-body: 'Sora', system-ui, -apple-system, 'Segoe UI', sans-serif;
                --radius: 6px;
            }
            :root[data-theme='playground-light'] {
                --bg: #FAFAF7; --bg-card: #FFFFFF; --border: #E4E4DD; --border-strong: #D4D4CB; --text: #17171C; --text-muted: #8A8A93; --accent: #1D5FBF; --accent-ink: #FAFAF7; --accent-dim: rgba(29, 95, 191, 0.08); --code-bg: #F1F1EC; --btn-bg: #F1F1EC; --btn-text: #17171C; --nested-bg: rgba(29, 95, 191, 0.05);
                --font-body: 'Sora', system-ui, -apple-system, 'Segoe UI', sans-serif;
                --radius: 6px;
            }

            /* --- ACCESSIBILITY --- */
            :focus { outline: none; }
            :focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; border-radius: 4px; }
            @media (prefers-reduced-motion: reduce) {
                *, *::before, *::after {
                    animation-duration: 0.01ms !important;
                    animation-iteration-count: 1 !important;
                    transition-duration: 0.01ms !important;
                    scroll-behavior: auto !important;
                }
            }

            ::-webkit-scrollbar { width: 8px; height: 8px; }
            ::-webkit-scrollbar-track { background: transparent; }
            ::-webkit-scrollbar-thumb { background: var(--border-strong); border-radius: 4px; }
            ::-webkit-scrollbar-thumb:hover { background: var(--text-muted); }

            body {
                font-family: var(--font-body);
                background: var(--bg); color: var(--text);
                padding: 0; margin: 0; line-height: 1.65;
                transition: background var(--transition), color var(--transition);
                overflow-x: hidden;
                -webkit-font-smoothing: antialiased;
            }
            ::selection { background: var(--accent); color: var(--accent-ink); }
            a { color: var(--accent); text-decoration: none; transition: color 0.15s; }
            a:hover { text-decoration: underline; text-underline-offset: 3px; }

            /* --- TOPBAR --- */
            .topbar {
                position: sticky; top: 0; z-index: 900;
                display: flex; align-items: center; gap: 12px;
                height: 56px; padding: 0 24px;
                background: color-mix(in srgb, var(--bg) 85%, transparent);
                backdrop-filter: blur(12px); -webkit-backdrop-filter: blur(12px);
                border-bottom: 1px solid var(--border);
            }
            .topbar .brand {
                font-weight: 700; font-size: 0.95rem; letter-spacing: -0.01em;
                color: var(--text); text-decoration: none;
            }
            .topbar .brand:hover { text-decoration: none; color: var(--accent); }
            .topbar .sep { color: var(--border-strong); user-select: none; }
            .topbar .crumb { font-size: 0.85rem; color: var(--text-muted); }
            .topbar .crumb:hover { color: var(--accent); text-decoration: none; }
            .topbar-spacer { flex: 1; }
            .theme-select {
                padding: 6px 10px; font-size: 0.8rem; font-family: var(--font-code);
                background: var(--btn-bg); color: var(--btn-text);
                border: 1px solid var(--border); border-radius: var(--radius);
                cursor: pointer; outline: none; transition: border-color var(--transition);
            }
            .theme-select:hover { border-color: var(--accent); }

            /* --- LAYOUT --- */
            .layout { display: flex; min-height: calc(100vh - 56px); width: 100%; }
            .sidebar {
                width: 300px; min-width: 300px;
                background: var(--bg-card); border-right: 1px solid var(--border);
                padding: 1.75rem 1.25rem 3rem;
                position: sticky; top: 56px; height: calc(100vh - 56px);
                overflow-y: auto; overflow-x: hidden;
            }
            .main-content { flex: 1; padding: 2.5rem 4rem 5rem; max-width: 1080px; margin: 0 auto; min-width: 0; }

            /* --- SIDEBAR --- */
            .sidebar h3 {
                font-size: 0.72rem; text-transform: uppercase; letter-spacing: 0.1em;
                color: var(--text-muted); margin: 2rem 0 0.75rem; font-weight: 600;
            }
            .sidebar h3:first-of-type { margin-top: 0; }
            .search-box { position: relative; margin-bottom: 0.5rem; }
            #search-input {
                width: 100%; padding: 0.6rem 0.9rem;
                border: 1px solid var(--border); border-radius: var(--radius);
                background: var(--bg); color: var(--text); outline: none;
                transition: border-color var(--transition); font-size: 0.88rem; font-family: var(--font-body);
            }
            #search-input::placeholder { color: var(--text-muted); }
            #search-input:focus { border-color: var(--accent); }
            #search-results {
                position: absolute; top: calc(100% + 6px); left: 0; right: 0;
                background: var(--bg-card); border: 1px solid var(--border-strong);
                border-radius: var(--radius); display: none; z-index: 1000;
                box-shadow: var(--shadow-lg); max-height: 420px; overflow-y: auto;
            }
            #search-results div { padding: 0.65rem 0.9rem; border-bottom: 1px solid var(--border); }
            #search-results div:last-child { border-bottom: none; }
            #search-results div:hover { background: var(--accent-dim); }
            #search-results a { display: block; color: var(--text); font-weight: 600; font-size: 0.88rem; }
            #search-results a:hover { text-decoration: none; color: var(--accent); }
            #search-results small { color: var(--text-muted); font-weight: 400; }

            .theme-toggles { display: flex; flex-wrap: wrap; gap: 0.5rem; }
            .theme-btn {
                cursor: pointer; padding: 6px 12px;
                border: 1px solid var(--border); border-radius: var(--radius);
                background: var(--btn-bg); color: var(--btn-text);
                font-size: 0.78rem; font-weight: 500; font-family: var(--font-body);
                transition: border-color var(--transition), color var(--transition);
            }
            .theme-btn:hover { border-color: var(--accent); color: var(--accent); }

            .nav-list { list-style: none; padding: 0; margin: 0; }
            .nav-list li { margin-bottom: 2px; }
            .nav-list a {
                color: var(--text-muted); font-weight: 500; font-size: 0.9rem;
                display: block; padding: 6px 10px; border-radius: var(--radius);
                border-left: 2px solid transparent;
            }
            .nav-list a:hover { color: var(--text); background: var(--nested-bg); text-decoration: none; }
            .nav-list a.active {
                color: var(--accent); background: var(--accent-dim);
                border-left-color: var(--accent); border-radius: 0 var(--radius) var(--radius) 0;
                font-weight: 600;
            }

            /* --- PAGE HEAD --- */
            .page-head { margin-bottom: 2.5rem; padding-bottom: 1.5rem; border-bottom: 1px solid var(--border); }
            .page-chip {
                display: inline-block; font-family: var(--font-code); font-size: 0.72rem; font-weight: 500;
                letter-spacing: 0.08em; text-transform: uppercase;
                color: var(--accent); background: var(--accent-dim);
                border: 1px solid var(--accent); border-radius: 4px;
                padding: 2px 8px; margin-bottom: 0.9rem;
            }
            .page-head h1 {
                font-size: 2.1rem; margin: 0 0 0.6rem; font-weight: 700;
                letter-spacing: -0.03em; color: var(--text); line-height: 1.2;
            }
            .page-sub { color: var(--text-muted); font-size: 0.95rem; margin: 0; }
            .page-sub code { font-family: var(--font-code); font-size: 0.85em; }

            /* --- STATS ROW --- */
            .stat-row { display: flex; flex-wrap: wrap; gap: 0.5rem; margin-top: 1.1rem; }
            .stat-chip {
                display: inline-flex; align-items: center; gap: 6px;
                font-family: var(--font-code); font-size: 0.78rem;
                color: var(--text-muted); background: var(--nested-bg);
                border: 1px solid var(--border); border-radius: 4px; padding: 3px 10px;
            }
            .stat-chip b { color: var(--text); font-weight: 600; }

            /* --- SYMBOLS HEADER --- */
            .symbols-header { display: flex; align-items: center; gap: 1rem; margin: 2.5rem 0 1rem; }
            .symbols-header h3 {
                margin: 0; flex: 1; font-size: 1rem; font-weight: 600;
                letter-spacing: -0.01em; border: none; padding: 0;
            }
            .filter-box { display: flex; align-items: center; gap: 0.6rem; margin: 0; font-size: 0.85rem; font-weight: 500; color: var(--text-muted); }

            /* Toggle Switch */
            .switch { position: relative; display: inline-block; width: 36px; height: 20px; }
            .switch input { opacity: 0; width: 0; height: 0; }
            .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--border-strong); transition: .25s; border-radius: 34px; }
            .slider:before { position: absolute; content: ""; height: 14px; width: 14px; left: 3px; bottom: 3px; background-color: var(--bg-card); transition: .25s; border-radius: 50%; box-shadow: var(--shadow); }
            input:checked + .slider { background-color: var(--accent); }
            input:focus-visible + .slider { outline: 2px solid var(--accent); outline-offset: 2px; }
            input:checked + .slider:before { transform: translateX(16px); }

            /* --- MODULE CARDS (home) --- */
            .module-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(240px, 1fr)); gap: 1rem; margin-top: 1.75rem; }
            .module-card {
                background: var(--bg-card); border: 1px solid var(--border);
                padding: 1.4rem 1.5rem; border-radius: var(--radius);
                transition: border-color var(--transition), background var(--transition);
                position: relative;
            }
            .module-card:hover { border-color: var(--accent); text-decoration: none; }
            .module-card .kind-badge { margin-bottom: 0.9rem; }
            .module-card a { font-size: 1.05rem; font-weight: 600; color: var(--text); }
            .module-card a:hover { color: var(--accent); text-decoration: none; }
            .module-card .card-count { display: block; margin-top: 0.5rem; font-family: var(--font-code); font-size: 0.75rem; color: var(--text-muted); }
            .module-card .card-arrow {
                position: absolute; top: 1.2rem; right: 1.2rem;
                color: var(--text-muted); font-size: 1rem;
                transition: color var(--transition), transform var(--transition);
            }
            .module-card:hover .card-arrow { color: var(--accent); transform: translateX(3px); }
            .hero-text { font-size: 1.05rem; color: var(--text-muted); margin: 0.75rem 0 0; max-width: 56ch; }

            /* --- FILE SYMBOL LIST (module index) --- */
            .top-level-sym { padding: 1rem 0; border-bottom: 1px solid var(--border); }
            .top-level-sym:last-child { border-bottom: none; }
            .top-level-sym hr { display: none; }
            .sym-header { display: flex; justify-content: space-between; align-items: center; gap: 1rem; }
            .sym-header > div:first-child { flex: 1; }
            .sym-header a b { color: var(--text); font-weight: 600; }
            .sym-header a:hover b { color: var(--accent); }
            .sym-header small { color: var(--text-muted); font-weight: 400; }
            .sym-loc { color: var(--text-muted); font-size: 0.8em; margin-left: 0.5rem; }
            .sym-loc i { font-family: var(--font-code); font-style: normal; color: var(--accent); opacity: 0.85; }

            .collapse-btn {
                background: none; border: 1px solid var(--border); border-radius: 4px;
                font-size: 0.65rem; cursor: pointer; color: var(--text-muted);
                transition: transform var(--transition), color var(--transition), border-color var(--transition);
                padding: 4px 8px; flex-shrink: 0;
            }
            .collapse-btn:hover { color: var(--accent); border-color: var(--accent); }
            .collapse-btn.expanded { transform: rotate(90deg); color: var(--accent); border-color: var(--accent); }
            .collapsible-content { padding: 0.5rem 0 0.25rem; }
            .collapsible-content ul { margin: 0.25rem 0 0; }

            /* --- DECLARATION NODES (file pages) --- */
            .node {
                background: var(--bg-card); border: 1px solid var(--border);
                padding: 1.5rem 1.75rem; margin-bottom: 1.25rem;
                border-radius: var(--radius); position: relative;
                transition: border-color var(--transition);
                scroll-margin-top: 72px;
            }
            .node:hover { border-color: var(--border-strong); }
            .node:target { border-color: var(--accent); box-shadow: 0 0 0 1px var(--accent); }
            .node.non-public { opacity: 0.72; }

            .node-header { padding: 0; display: flex; align-items: center; flex-wrap: wrap; gap: 0.5rem; }
            .node-title {
                font-size: 1.35rem; font-weight: 700; margin: 0;
                color: var(--text); font-family: var(--font-code); letter-spacing: -0.02em;
            }
            .node-anchor {
                margin-left: 4px; color: var(--text-muted); opacity: 0;
                transition: opacity var(--transition); font-size: 0.85rem; text-decoration: none;
            }
            .node-header:hover .node-anchor { opacity: 1; }
            .node-anchor:hover { color: var(--accent); }

            .badge-row { display: flex; gap: 0.5rem; flex-wrap: wrap; margin-top: 0.6rem; align-items: center; }
            .kind-badge {
                display: inline-block;
                background: var(--accent-dim); color: var(--accent);
                border: 1px solid var(--accent);
                padding: 1px 8px; border-radius: 4px;
                font-family: var(--font-code); font-size: 0.68rem; font-weight: 600;
                text-transform: uppercase; letter-spacing: 0.07em;
            }
            .attr-badge {
                display: inline-block;
                border: 1px solid var(--border-strong); color: var(--text-muted);
                padding: 1px 8px; border-radius: 4px;
                font-family: var(--font-code); font-size: 0.68rem; font-weight: 500;
            }
            .attr-badge.badge-private { border-style: dashed; }
            .attr-badge.badge-constructor { border-color: var(--accent); color: var(--accent); }
            .attr-badge.badge-implicit { color: var(--text-muted); border-color: var(--border-strong); }
            .attr-badge.badge-override { border-color: var(--accent); color: var(--accent); }
            .extension-tag {
                background: var(--accent-dim); color: var(--accent); border: 1px solid var(--accent);
                padding: 1px 8px; border-radius: 4px;
                font-family: var(--font-code); font-size: 0.68rem;
                text-transform: uppercase; font-weight: 600; letter-spacing: 0.07em;
            }

            .signature {
                font-family: var(--font-code); background: var(--code-bg);
                padding: 1rem 1.15rem; border-radius: var(--radius); margin: 1.1rem 0;
                font-size: 0.88rem; border: 1px solid var(--border);
                overflow-x: auto; white-space: pre-wrap; word-break: break-word;
                position: relative; line-height: 1.6;
            }
            .copy-btn {
                position: absolute; top: 8px; right: 8px;
                padding: 3px 10px; font-size: 0.7rem; font-family: var(--font-code);
                background: var(--btn-bg); color: var(--text-muted);
                border: 1px solid var(--border); border-radius: 4px;
                cursor: pointer; opacity: 0; transition: opacity var(--transition), color var(--transition), border-color var(--transition);
            }
            .signature:hover .copy-btn { opacity: 1; }
            .copy-btn:hover { color: var(--accent); border-color: var(--accent); }
            .copy-btn.copied { color: var(--accent); border-color: var(--accent); opacity: 1; }

            .tok-kwd { color: var(--accent); font-weight: 600; }
            .tok-type { color: #b9a3ff; }
            .tok-str { color: #7fd1b9; }
            .tok-com { color: var(--text-muted); font-style: italic; }
            .tok-fn { color: var(--text); font-weight: 600; }
            :root[data-theme='light'] .tok-type, :root[data-theme='paper'] .tok-type { color: #6d4fc4; }
            :root[data-theme='light'] .tok-str, :root[data-theme='paper'] .tok-str { color: #1f7a5c; }
            :root[data-theme='light'] .tok-kwd, :root[data-theme='paper'] .tok-kwd { color: var(--accent); }
            :root[data-theme='playground'] .tok-str, :root[data-theme='playground-light'] .tok-str { color: #7fd1b9; }
            :root[data-theme='playground-light'] .tok-str { color: #1f7a5c; }

            .comment { color: var(--text); font-size: 0.95rem; margin-top: 1.1rem; padding-top: 1rem; border-top: 1px solid var(--border); white-space: pre-wrap; }
            .comment h2, .comment h3 { border: none; margin-top: 1.25rem; padding: 0; font-size: 1.05rem; }
            .doc-main { margin-bottom: 0.75rem; }
            .doc-main p { margin: 0 0 0.5rem; }
            .doc-section { margin-top: 1rem; }
            .doc-section h4 {
                margin: 0.75rem 0 0.4rem; color: var(--text-muted); text-transform: uppercase;
                font-family: var(--font-code); font-size: 0.7rem; letter-spacing: 0.08em; font-weight: 600;
            }
            .doc-list { list-style: none; padding: 0; margin: 0; }
            .doc-list li { margin-bottom: 0.4rem; padding-left: 0.9rem; border-left: 2px solid var(--border-strong); }
            .doc-list b { color: var(--accent); font-family: var(--font-code); font-weight: 600; font-size: 0.9em; }

            .nested-container {
                margin-top: 1.25rem; padding: 1.1rem 1.25rem; border-radius: var(--radius);
                background: var(--nested-bg); border-left: 2px solid var(--accent);
                display: flex; flex-direction: column; gap: 1rem;
            }
            .nested-container .node { margin-bottom: 0; padding: 1.1rem 1.3rem; }
            .nested-container .node-title { font-size: 1.1rem; }
            .nested-container .signature { font-size: 0.8rem; padding: 0.85rem 1rem; }

            /* --- EMPTY STATE --- */
            .empty-state {
                text-align: center; padding: 3.5rem 1.5rem;
                border: 1px dashed var(--border-strong); border-radius: var(--radius);
                color: var(--text-muted); margin: 2rem 0;
            }
            .empty-state .empty-icon { font-family: var(--font-code); font-size: 1.4rem; color: var(--border-strong); display: block; margin-bottom: 0.75rem; }
            .empty-state p { margin: 0; font-size: 0.92rem; }

            /* --- MISC --- */
            .breadcrumb { font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1.25rem; font-weight: 500; }
            .breadcrumb a { color: var(--text-muted); }
            .breadcrumb a:hover { color: var(--accent); }
            .git-link {
                color: var(--text-muted); display: inline-flex; align-items: center;
                padding: 5px; border-radius: var(--radius); border: 1px solid var(--border);
                transition: all var(--transition); opacity: 0.55; flex-shrink: 0;
            }
            .git-link:hover { opacity: 1; border-color: var(--accent); color: var(--accent); }
            .git-icon { width: 15px; height: 15px; }
            .dependency-graph {
                margin: 1.5rem 0 2.25rem; padding: 1.25rem 1.75rem;
                background: var(--bg-card); border: 1px solid var(--border); border-radius: var(--radius);
                overflow: auto;
            }
            .dependency-graph h3 { margin: 0 0 1rem; padding: 0; border: none; font-size: 1rem; }
            .mermaid { background: transparent !important; }
            .mermaid svg { overflow: visible !important; display: block; max-width: 100%; }

            .debug-toggle {
                font-family: var(--font-code); font-size: 0.72rem; color: var(--text-muted);
                cursor: pointer; border: 1px solid var(--border); background: none;
                margin: 4rem auto 0; padding: 8px 16px; border-radius: 4px;
                display: block; transition: color var(--transition), border-color var(--transition);
            }
            .debug-toggle:hover { color: var(--accent); border-color: var(--accent); }
            #debug-info {
                padding: 1.1rem 1.3rem; background: var(--code-bg); border-radius: var(--radius);
                font-family: var(--font-code); font-size: 0.78rem; margin-top: 1rem;
                color: var(--text-muted); border: 1px solid var(--border); word-break: break-all;
            }

            h2, h3 { margin-top: 2.75rem; border-bottom: 1px solid var(--border); padding-bottom: 0.6rem; font-weight: 700; letter-spacing: -0.02em; }

            /* --- FOOTER --- */
            .doc-footer {
                border-top: 1px solid var(--border); margin-top: 5rem; padding: 1.5rem 0 0;
                display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 0.75rem;
                font-size: 0.8rem; color: var(--text-muted);
            }
            .doc-footer a { color: var(--text-muted); }
            .doc-footer a:hover { color: var(--accent); }

            #back-top {
                position: fixed; bottom: 1.5rem; right: 1.5rem; z-index: 800;
                width: 38px; height: 38px; display: flex; align-items: center; justify-content: center;
                background: var(--bg-card); color: var(--text-muted);
                border: 1px solid var(--border-strong); border-radius: var(--radius);
                cursor: pointer; opacity: 0; pointer-events: none;
                transition: opacity var(--transition), color var(--transition), border-color var(--transition);
                font-size: 0.9rem;
            }
            #back-top.visible { opacity: 1; pointer-events: auto; }
            #back-top:hover { color: var(--accent); border-color: var(--accent); }

            /* --- RESPONSIVE --- */
            @media (max-width: 1024px) {
                .layout { flex-direction: column; }
                .sidebar { width: 100%; min-width: 0; height: auto; max-height: 42vh; position: static; border-right: none; border-bottom: 1px solid var(--border); }
                .main-content { padding: 2rem 1.5rem 4rem; }
                .page-head h1 { font-size: 1.6rem; }
                .node { padding: 1.15rem 1.25rem; }
                .topbar { padding: 0 16px; }
                .topbar .crumb { display: none; }
                .topbar .sep { display: none; }
            }
            @media (max-width: 640px) {
                .page-head h1 { font-size: 1.35rem; }
                .node-title { font-size: 1.1rem; }
                .signature { font-size: 0.78rem; }
                .copy-btn { opacity: 1; }
                .module-grid { grid-template-columns: 1fr; }
                .theme-select { max-width: 130px; }
                .stat-row { gap: 0.35rem; }
            }
        """
    }}

}
