
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
            main_desc.append_view(line);
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
                html.append_view(p);
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
    search.append_view(mod_name);
    search.append_view("|");
    return std::string(natives).to_view().contains(search.to_view());
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
            if (kwds_view.contains(word)) span_class = std::string_view("tok-kwd");
            else if (types_view.contains(word)) span_class = std::string_view("tok-type");
            
            if (!span_class.empty()) {
                html.append_view("<span class='");
                html.append_view(span_class);
                html.append_view("'>");
                html.append_view(word);
                html.append_view("</span>");
            } else {
                html.append_view(word);
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
        mod_dir.append_view(mod_name);
        
        res = fs::create_dir_all(mod_dir.data());
        if (res is std::Result.Err) {
            var Err(e) = res else unreachable;
            printf("Error creating module directory %s: %s\n", mod_dir.data(), e.message().data());
            return;
        }

        var count = module.getFileCount();
        for (var i = 0u; i < count; i++) {
            var file_meta = module.getFile(i);
            self.generate_file_docs(file_meta, mod_dir.to_view(), mod_name);
        }

        self.generate_module_index(mod_name, mod_dir.to_view());
    }

    func generate_module_index(&mut self, mod_name : std::string_view, mod_dir : std::string_view) {
        var rel_root = get_relative_root(1);
        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta name='description' content='Chemical API Documentation for module ");
        html.append_view(mod_name);
        html.append_view("'><title>");
        html.append_view(mod_name);
        html.append_view(" Index</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style><script>function setTheme(t){document.documentElement.setAttribute('data-theme',t);localStorage.setItem('refgen-theme',t);}const t=localStorage.getItem('refgen-theme')||'dark';document.documentElement.setAttribute('data-theme',t);</script></head><body><div class='layout'>");
        
        html.append_string(self.generate_sidebar(rel_root.to_view()).copy());

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='breadcrumb'><a href='../index.html'>Home</a> / ");
        html.append_view(mod_name);
        html.append_view("</div>");
        html.append_view("<div class='header'><h1>Module: ");
        html.append_view(mod_name);
        html.append_view("</h1></div>");
        
        // Dependency Graph Visualization
        html.append_view("<div class='dependency-graph'>");
        html.append_view("<h3 style='margin-top:0;padding-bottom:0.5rem;border:none;'>Module Dependencies</h3>");
        html.append_view("<div class='mermaid'>");
        html.append_string(self.generate_dependency_graph(mod_name));
        html.append_view("</div></div>");

        html.append_view("<div class='symbols-header'>");
        html.append_view("<h3>Symbols</h3>");
        html.append_view("<div class='filter-box' style='margin-bottom:0;'><label class='switch'><input type='checkbox' id='public-only' onchange='applyFilter()'><span class='slider round'></span></label> <span>Public Only</span></div>");
        html.append_view("<button class='theme-btn' onclick='toggleExpandAll(true)'>Expand All</button>");
        html.append_view("<button class='theme-btn' onclick='toggleExpandAll(false)'>Collapse All</button>");
        html.append_view("</div><div class='symbol-tree'>");
        
        // Pass 1: Top-level symbols
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (sym.mod_name.to_view().equals(mod_name) && sym.parent_name.empty()) {
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
                html.append_view("<span style='color: var(--text-muted); font-size: 0.8em; margin-left: 0.5rem;'>in <i>");
                html.append_view(sym.filename.to_view());
                html.append_view("</i></span> ");
                
                var has_children = false;
                for (var j = 0u; j < self.index.size(); j++) {
                    var child = self.index.get_ptr(j);
                    if (child.mod_name.to_view().equals(mod_name) && child.parent_name.to_view().equals(sym.name.to_view())) {
                        has_children = true;
                        break;
                    }
                }
                
                if (sym.access != AccessSpecifier.Public) {
                    html.append_view("<span class='attr-badge' style='background: #64748b; color: white;'>Private</span>");
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
                        if (child.mod_name.to_view().equals(mod_name) && child.parent_name.to_view().equals(sym.name.to_view())) {
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
        
        html.append_view("</div></div></div>");
        html.append_string(self.get_js(rel_root.to_view()));
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
                        html.append_view(m_name);
                        html.append_view("/src/");
                        html.append_view(rel_path);
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
            if (m.getName().equals(name)) return m;
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
            mermaid.append_view(mod_name);
            mermaid.append_view(" --> ");
            mermaid.append_view(d_name);
            mermaid.append_view("[");
            mermaid.append_view(d_name);
            mermaid.append_view("]\n");
            // Check if already visited to avoid infinite recursion
            var already = false;
            for (var j = 0u; j < visited.size(); j++) {
                if (visited.get(j).to_view().equals(d_name)) {
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
        mermaid.append_view(mod_name);
        mermaid.append_view("[");
        mermaid.append_view(mod_name);
        mermaid.append_view("]\n");
        
        var visited = std::vector<std::string>();
        visited.push_back(std::string(mod_name.data(), mod_name.size()));
        self.add_module_deps(mod, mermaid, visited);
        return mermaid;
    }

    func generate_sidebar(&mut self, rel_root : &std::string_view) : &std::string {
        if (self.sidebar_cache.contains(std::string(rel_root.data(), rel_root.size()))) {
            return *self.sidebar_cache.get_ptr(std::string(rel_root.data(), rel_root.size()));
        }

        var html = std::string("<div class='sidebar'>");
        html.append_view("<div class='search-box'><input type='text' id='search-input' placeholder='Search...' oninput='searchSymbols()'><div id='search-results'></div></div>");
        
        html.append_view("<h3>Themes</h3><div class='theme-toggles'>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('light')\">Light</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('dark')\">Dark</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('paper')\">Paper</button>");
        html.append_view("</div>");
        

        html.append_view("<h3>Modules</h3><ul class='nav-list'>");
        var last_mod = std::string("");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (!sym.mod_name.equals(last_mod)) {
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
        self.sidebar_cache.insert(std::string(rel_root.data(), rel_root.size()), html.copy());
        return *self.sidebar_cache.get_ptr(std::string(rel_root.data(), rel_root.size()));
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

        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta name='description' content='Chemical API Documentation for ");
        html.append_view(filename);
        html.append_view("'><title>");
        html.append_view(filename);
        html.append_view(" - Chemical API</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style><script>function setTheme(t){document.documentElement.setAttribute('data-theme',t);localStorage.setItem('refgen-theme',t);}const t=localStorage.getItem('refgen-theme')||'dark';document.documentElement.setAttribute('data-theme',t);</script></head><body><div class='layout'>");

        html.append_string(self.generate_sidebar(rel_root.to_view()).copy());

        html.append_view("<div class='main-content'>");
        
        // Breadcrumbs
        html.append_view("<div class='breadcrumb'><a href='");
        html.append_view(rel_root.to_view());
        html.append_view("/index.html'>Home</a> / <a href='./index.html'>");
        html.append_view(mod_name);
        html.append_view("</a> / ");
        html.append_view(filename);
        html.append_view("</div>");

        html.append_view("<div class='header'><h1>");
        html.append_view(filename);
        html.append_view("</h1>");
        // No line number for file header? Or line 1?
        self.render_github_link(abs_path, 1, html);
        html.append_view("</div>");
        html.append_view("<div class='symbols-header'><h3>Declarations</h3><div class='filter-box' style='margin-bottom:0;'><label class='switch'><input type='checkbox' id='public-only' onchange='applyFilter()'><span class='slider round'></span></label> <span>Public Only</span></div></div>");

        var i = 0u;
        while (i < nodes.size()) {
            var node = nodes.get(i);
            self.document_node(node, html, tokens, rel_root.to_view(), abs_path);
            i++;
        }

        // Debug Info section
        html.append_view("<button class='debug-toggle' onclick='toggleDebug()'>Show Debug Info</button>");
        html.append_view("<div id='debug-info' style='display:none;'>Generated from: ");
        html.append_view(abs_path);
        html.append_view("<br>File ID: ");
        var f_id_str = std::string("");
        f_id_str.append_uinteger(file_id as ubigint);
        html.append_view(f_id_str.to_view());
        html.append_view("</div>");

        html.append_view("</div></div>");
        html.append_string(self.get_js(rel_root.to_view()));
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
                var sym = self.find_symbol(name);
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
                    html.append_view(name);
                    html.append_view("'>");
                    html.append_view(name);
                    html.append_view("</a>");
                } else {
                    html.append_view(name);
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
        html.append_view(name);
        html.append_view("'>");
        
        // Header
        html.append_view("<div class='node-header' style='flex-direction: column; align-items: flex-start;'>");
        
        html.append_view("<div style='display: flex; justify-content: space-between; align-items: baseline; width: 100%;'>");
        html.append_view("<span class='node-title'>");
        html.append_view(name);
        html.append_view("</span>");
        
        if(true){
            var encoded_loc = node.getEncodedLocation();
            var loc_data = self.ctx.decodeLocation(encoded_loc);
            self.render_github_link(abs_path, loc_data.lineStart + 1, html);
        }
        
        html.append_view("</div>");

        html.append_view("<div style='display: flex; gap: 0.5rem; flex-wrap: wrap; margin-top: 0.5rem; align-items: center;'>");

        if (access == AccessSpecifier.Public) html.append_view("<span class='attr-badge'>Public</span>");

        html.append_view("<span class='kind-badge' style='margin-right: 0;'>");
        html.append_view(kind_label);
        html.append_view("</span>");

        if (kind == ASTNodeKind.FunctionDecl || kind == ASTNodeKind.GenericFuncDecl) {
            var header_decl : *FunctionDeclaration = null;
            if (kind == ASTNodeKind.FunctionDecl) {
                header_decl = node as *FunctionDeclaration;
            } else {
                header_decl = (node as *GenericFuncDecl).getMasterImpl();
            }
            if (header_decl != null) {
                var attrs : FuncDeclAttributesCBI = zeroed<FuncDeclAttributesCBI>();
                header_decl.getAttributes(&mut attrs);

                if (header_decl.isExtensionFn()) {
                    html.append_view("<span class='extension-tag' style='margin-left: 0;'>Extension</span>");
                }

                if (attrs.is_constructor_fn) {
                    if (attrs.is_implicit) html.append_view("<span class='attr-badge' style='background: #8b5cf6; border-color: #8b5cf6; color: white;'>Implicit Constructor</span>");
                    else html.append_view("<span class='attr-badge' style='background: #10b981; border-color: #10b981; color: white;'>Constructor</span>");
                }
                if (attrs.is_override) {
                    html.append_view("<span class='attr-badge' style='background: #f59e0b; border-color: #f59e0b; color: white;'>Override</span>");
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

            var attrs : FuncDeclAttributesCBI = zeroed<FuncDeclAttributesCBI>();
            decl.getAttributes(&mut attrs);
            
            html.append_view("<span class='tok-kwd'>func</span> ");
            html.append_view(name);
            
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
            
            html.append_view(name);
            
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
            html.append_view(name);
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
            html.append_view(name);
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
            html.append_view(name);
        } else if (kind == ASTNodeKind.TypealiasStmt) {
            var stmt = node as *TypealiasStatement;
            html.append_view("<span class='tok-kwd'>type</span> ");
            html.append_view(name);
            html.append_view(" = ");
            self.render_type(stmt.getActualType(), html, rel_root);
        } else if (kind == ASTNodeKind.StructMember || kind == ASTNodeKind.VariantMember) {
            var member = node as *BaseDefMember;
            html.append_view("<span class='tok-kwd'>var</span> ");
            html.append_view(name);
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
        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta name='description' content='Chemical Language Core API Reference'>");
        html.append_view("<title>Chemical API Documentation</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style><script>function setTheme(t){document.documentElement.setAttribute('data-theme',t);localStorage.setItem('refgen-theme',t);}const t=localStorage.getItem('refgen-theme')||'dark';document.documentElement.setAttribute('data-theme',t);</script></head><body><div class='layout'>");
        
        html.append_string(self.generate_sidebar(rel_root.to_view()).copy());

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='header'><h1>Chemical API Reference</h1></div>");
        html.append_view("<p class='hero-text'>Explore professional documentation for the Chemical Language core libraries and standard modules.</p>");
        
        html.append_view("<div class='module-grid'>");
        var last_mod = std::string("");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (!sym.mod_name.equals(last_mod)) {
                html.append_view("<div class='module-card'>");
                html.append_view("<div class='kind-badge'>Module</div>");
                html.append_view("<br><br><a href='./");
                html.append_view(sym.mod_name.to_view());
                html.append_view("/index.html'>");
                html.append_view(sym.mod_name.to_view());
                html.append_view("</a></div>");
                last_mod = sym.mod_name.copy();
            }
        }
        html.append_view("</div></div></div>");
        html.append_string(self.get_js(rel_root.to_view()));
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
            </script>
            <script type="module">
            import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs';
            mermaid.initialize({
                startOnLoad: true,
                theme: "base",
                themeVariables: {
                    primaryColor: "var(--bg-card)",
                    primaryTextColor: "var(--text)",
                    lineColor: "var(--accent)",
                    secondaryColor: "var(--code-bg)"
                }
            });
            </script>
        """);
        // Replace REL_ROOT_VAR with actual rel_root
        var res = std::string("");
        var raw = s.to_view();
        var placeholder = std::string_view("${REL_ROOT_VAR}");
        var last_idx = 0u;
        while (true) {
            var sub = raw.subview(last_idx, raw.size());
            var p_idx = sub.find(placeholder);
            if (p_idx == -1u) {
                res.append_view(sub);
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
            :root {
                --transition: 0.3s cubic-bezier(0.4, 0, 0.2, 1);
                --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
                --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
                --radius: 12px;
            }
            * { box-sizing: border-box; scroll-behavior: smooth; }
            :root[data-theme='light'] {
                --bg: #f8fafc; --bg-card: #ffffff; --border: #e2e8f0; --text: #0f172a; --text-muted: #64748b; --accent: #3b82f6; --code-bg: #f1f5f9; --btn-bg: #ffffff; --btn-text: #0f172a; --nested-bg: rgba(0,0,0,0.02);
            }
            :root[data-theme='dark'] {
                --bg: #0b0f19; --bg-card: #111827; --border: #1f2937; --text: #f3f4f6; --text-muted: #9ca3af; --accent: #60a5fa; --code-bg: #1f2937; --btn-bg: #1f2937; --btn-text: #f3f4f6; --nested-bg: rgba(255,255,255,0.03);
            }
            :root[data-theme='paper'] {
                --bg: #f4f1ea; --bg-card: #fdfcf9; --border: #e2ddd3; --text: #433f38; --text-muted: #7c7467; --accent: #8b5e34; --code-bg: #e9e4d9; --btn-bg: #fdfcf9; --btn-text: #433f38; --nested-bg: rgba(0,0,0,0.03);
            }
            ::-webkit-scrollbar { width: 10px; height: 10px; }
            ::-webkit-scrollbar-track { background: var(--bg); }
            ::-webkit-scrollbar-thumb { background: var(--border); border-radius: 5px; }
            ::-webkit-scrollbar-thumb:hover { background: var(--text-muted); }
            body { 
                font-family: 'Outfit', 'Inter', system-ui, sans-serif; 
                background: var(--bg); color: var(--text); padding: 0; margin: 0; line-height: 1.6;
                transition: background var(--transition), color var(--transition);
                overflow-x: hidden;
            }
            .layout { display: flex; min-height: 100vh; width: 100%; }
            .sidebar { 
                width: 320px; min-width: 320px; background: var(--bg-card); border-right: 1px solid var(--border); 
                padding: 2.5rem 1.5rem; position: sticky; top: 0; height: 100vh; overflow-y: auto; overflow-x: hidden;
                box-shadow: 4px 0 24px rgba(0,0,0,0.02);
            }
            .main-content { flex: 1; padding: 3rem 5rem; max-width: 1100px; margin: 0 auto; }
            .header { 
                display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 3rem;
                padding-bottom: 1.5rem; border-bottom: 2px solid var(--border);
            }
            .header h1 { font-size: 3rem; margin: 0; font-weight: 800; letter-spacing: -0.05em; color: var(--accent); }
            .search-box { position: relative; margin-bottom: 3rem; }
            #search-input { 
                width: 100%; padding: 0.85rem 1.25rem; border: 1.5px solid var(--border); border-radius: var(--radius);
                background: var(--bg); color: var(--text); outline: none; transition: all var(--transition);
                font-size: 1rem;
            }
            #search-input:focus { border-color: var(--accent); box-shadow: 0 0 0 4px rgba(59, 130, 246, 0.1); }
            #search-results {
                position: absolute; top: 105%; left: 0; right: 0; background: var(--bg-card);
                border: 1px solid var(--border); border-radius: var(--radius); display: none; z-index: 1000;
                box-shadow: var(--shadow-lg); max-height: 480px; overflow-y: auto; backdrop-filter: blur(8px);
            }
            #search-results div { padding: 1rem; border-bottom: 1px solid var(--border); transition: background 0.15s; }
            #search-results div:hover { background: var(--code-bg); }
            #search-results a { display: block; color: var(--text); font-weight: 600; }
            
            .node-header { padding-bottom: 0.5rem; display: flex; align-items: center; flex-wrap: wrap; gap: 0.5rem; }
            .node { 
                background: var(--bg-card); border: 1px solid var(--border); padding: 2.5rem; 
                margin-bottom: 3rem; border-radius: 16px; box-shadow: var(--shadow);
                position: relative; transition: transform var(--transition), box-shadow var(--transition);
            }
            .node:hover { transform: translateY(-2px); box-shadow: var(--shadow-lg); }
            
            .nested-container { 
                margin-top: 1.5rem; padding: 1.5rem; border-radius: var(--radius);
                background: var(--nested-bg); border-left: 4px solid var(--accent);
                display: flex; flex-direction: column; gap: 2rem;
            }
            .nested-container .node { margin-bottom: 0; padding: 1.5rem; border-radius: 12px; }

            .kind-badge { 
                background: var(--accent); color: white; padding: 4px 12px; border-radius: 6px; 
                font-size: 0.75rem; font-weight: 700; text-transform: uppercase;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            .attr-badge { border: 1.5px solid var(--accent); color: var(--accent); padding: 3px 8px; border-radius: 6px; font-size: 0.75rem; font-weight: 600; }
            .node-title { font-size: 2.15rem; font-weight: 800; margin: 0.75rem 0; color: var(--text); }
            .signature { 
                font-family: 'JetBrains Mono', monospace; background: var(--code-bg); padding: 1.25rem;
                border-radius: 10px; margin: 2rem 0; font-size: 1rem; border: 1px solid var(--border);
                overflow-x: auto; white-space: pre-wrap;
            }
            .tok-kwd { color: #ef4444; font-weight: 700; }
            .tok-type { color: #8b5cf6; }
            .tok-str { color: #10b981; }
            .tok-com { color: var(--text-muted); font-style: italic; }
            .tok-fn { color: var(--accent); font-weight: 700; }
            
            .comment { color: var(--text); font-size: 1.1rem; margin-top: 2rem; padding: 1rem 0; border-top: 1px solid var(--border); white-space: pre-wrap; }
            .comment h2, .comment h3 { border: none; margin-top: 1.5rem; padding: 0; }
            
            .theme-toggles { display: flex; flex-wrap: wrap; gap: 0.75rem; margin-top: 1rem; }
            .theme-btn { 
                cursor: pointer; padding: 8px 16px; border: 1.5px solid var(--border); border-radius: 10px; 
                background: var(--btn-bg); color: var(--btn-text); font-size: 0.9rem; font-weight: 600;
                transition: all var(--transition);
            }
            .theme-btn:hover { border-color: var(--accent); color: var(--accent); transform: translateY(-1px); }
            
            a { color: var(--accent); text-decoration: none; transition: all 0.2s; }
            a:hover { filter: brightness(1.2); }
            .extension-tag { 
                background: var(--accent); color: white; padding: 2px 8px; border-radius: 4px; font-size: 0.7rem; text-transform: uppercase; font-weight: 800; vertical-align: middle; margin-left: 8px;
            }
            .git-link { 
                color: var(--text-muted); display: inline-flex; align-items: center; justify-content: center;
                padding: 6px; border-radius: 8px; border: 1.5px solid var(--border); transition: all var(--transition);
                margin-left: auto; opacity: 0.4;
            }
            .git-link:hover, .node-header:hover .git-link, .header:hover .git-link { opacity: 1; border-color: var(--accent); background: var(--code-bg); color: var(--accent); }
            .git-icon { opacity: 0.9; width: 18px; height: 18px; }
            .nav-list { list-style: none; padding: 0; margin: 0; }
            .nav-list li { margin-bottom: 0.85rem; }
            .nav-list a { color: var(--text); font-weight: 500; font-size: 0.95rem; display: block; padding: 4px 0; }
            .nav-list a:hover { color: var(--accent); padding-left: 4px; }
            
            .breadcrumb { font-size: 1rem; color: var(--text-muted); margin-bottom: 1rem; font-weight: 500; }
            .breadcrumb a { color: var(--text-muted); }
            .breadcrumb a:hover { color: var(--accent); }
            
            .symbols-header { display: flex; align-items: center; gap: 1rem; margin-bottom: 1rem; }
            .symbols-header h3 { margin: 0; flex: 1; }
            
            .dependency-graph { 
                margin: 0.5rem 0 2rem 0; padding: 1rem 2rem; background: var(--bg-card); 
                border: 1px solid var(--border); border-radius: var(--radius);
                box-shadow: var(--shadow); overflow: auto;
            }
            .dependency-graph h3 { margin-top: 0; padding-bottom: 0.5rem; border: none; }
            .mermaid { background: transparent !important; }
            .mermaid svg { overflow: visible !important; display: block; }

            .doc-main { margin-bottom: 1.5rem; font-style: normal; }
            .doc-section { margin-top: 1.5rem; }
            .doc-section h4 { margin: 1rem 0 0.5rem 0; color: var(--text-muted); text-transform: uppercase; font-size: 0.8rem; letter-spacing: 0.05em; }
            .doc-list { list-style: none; padding: 0; margin: 0; }
            .doc-list li { margin-bottom: 0.5rem; padding-left: 1rem; border-left: 2px solid var(--border); }
            .doc-list b { color: var(--accent); }
            
            .debug-toggle {
                font-size: 0.8rem; opacity: 0.4; cursor: pointer; border: 1.5px solid var(--border);
                background: none; color: var(--text); margin-top: 6rem; padding: 10px 20px; border-radius: 20px;
                display: block; margin-left: auto; margin-right: auto; transition: all var(--transition);
            }
            
            /* Toggle Switch */
            .filter-box { display: flex; align-items: center; gap: 0.75rem; margin-bottom: 2rem; font-weight: 600; }
            .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
            .switch input { opacity: 0; width: 0; height: 0; }
            .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--border); transition: .4s; border-radius: 34px; }
            .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .4s; border-radius: 50%; }
            input:checked + .slider { background-color: var(--accent); }
            input:focus + .slider { box-shadow: 0 0 1px var(--accent); }
            input:checked + .slider:before { transform: translateX(20px); }
            .debug-toggle:hover { opacity: 1; border-color: var(--accent); color: var(--accent); }
            #debug-info { 
                display: none; padding: 1.5rem; background: var(--code-bg); border-radius: var(--radius); 
                font-family: 'JetBrains Mono', monospace; font-size: 0.85rem; margin-top: 1.5rem; color: var(--text-muted);
                border: 1px solid var(--border);
            }
            h2, h3 { margin-top: 4rem; border-bottom: 2px solid var(--border); padding-bottom: 0.75rem; font-weight: 800; letter-spacing: -0.025em; }
            
            .module-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 1.5rem; margin-top: 1.5rem; }
            .module-card { 
                background: var(--bg-card); border: 1px solid var(--border); padding: 2rem; border-radius: 16px;
                box-shadow: var(--shadow); transition: all var(--transition); text-align: center;
            }
            .module-card:hover { transform: translateY(-4px); box-shadow: var(--shadow-lg); border-color: var(--accent); }
            .module-card a { font-size: 1.5rem; font-weight: 800; color: var(--text); }
            .hero-text { font-size: 1.25rem; color: var(--text-muted); margin-bottom: 3rem; }
            
            .sym-header { display: flex; justify-content: space-between; align-items: center; }
            .collapse-btn { 
                background: none; border: none; font-size: 0.8rem; cursor: pointer; color: var(--text-muted);
                transition: transform var(--transition); padding: 4px 8px;
            }
            .collapse-btn.expanded { transform: rotate(90deg); color: var(--accent); }
            .collapsible-content { padding-bottom: 0.5rem; }
        """;
    }
}

}
