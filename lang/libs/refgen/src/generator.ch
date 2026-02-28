
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

func is_alphanum(c : char) : bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

func highlight_chemical(code : std::string_view) : std::string {
    var kwds = "func|var|const|struct|enum|namespace|public|private|if|else|while|for|return|break|continue|switch|case|default|import|using|as|in|true|false|null|defer|unsafe|impl|interface|union|bitfield|comptime|type|extend|trait|mut|self|Self|this|is|dyn|loop|new|destruct|dealloc|delete|provide|init|try|catch|throw|from|do|sizeof|alignof|protected|internal|any|void|alias|variant";
    var types = "i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|bool|char|int|long|float|double|uint|ulong|short|ushort|uchar";
    
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
            if (std::string(kwds).to_view().contains(word)) span_class = std::string_view("tok-kwd");
            else if (std::string(types).to_view().contains(word)) span_class = std::string_view("tok-type");
            
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

    func index_node_recursive(&mut self, node : *ASTNode, file_id : uint, mod_name : std::string_view) {
        var name = get_node_name(node);
        if (name.size() > 0) {
            self.index.push_back(SymbolInfo {
                name = std::string(name.data(), name.size()),
                kind = node.getKind(),
                file_id = file_id,
                mod_name = std::string(mod_name.data(), mod_name.size()),
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
                    self.index_node_recursive(children.get(i), file_id, mod_name);
                }
            }
        } else if (kind == ASTNodeKind.StructDecl) {
            var def = node as *StructDefinition;
            var funcs = def.getFunctions();
            if (funcs != null) {
                for (var i = 0u; i < funcs.size(); i++) {
                    self.index_node_recursive(funcs.get(i), file_id, mod_name);
                }
            }
            // Members usually don't need top-level indexing for search unless they are special,
            // but for deep search we could index them too. However, let's stick to functions/types for now.
        } else if (kind == ASTNodeKind.InterfaceDecl) {
            var def = node as *InterfaceDefinition;
            var funcs = def.getFunctions();
            if (funcs != null) {
                for (var i = 0u; i < funcs.size(); i++) {
                    self.index_node_recursive(funcs.get(i), file_id, mod_name);
                }
            }
        }
        // Add more recursive containers if needed (variants, unions, etc.)
    }

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
                self.index_node_recursive(nodes.get(j), file_id, mod_name);
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
        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>");
        html.append_view(mod_name);
        html.append_view(" Index</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style></head><body><div class='layout'>");
        
        html.append_string(self.generate_sidebar(rel_root.to_view()));

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='breadcrumb'><a href='../index.html'>Home</a> / ");
        html.append_view(mod_name);
        html.append_view("</div>");
        html.append_view("<div class='header'><h1>Module: ");
        html.append_view(mod_name);
        html.append_view("</h1></div>");

        html.append_view("<h3>Symbols in this module</h3><ul class='nav-list'>");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (sym.mod_name.to_view().equals(mod_name)) {
                html.append_view("<li><a href='./");
                var f_id = std::string("");
                f_id.append_uinteger(sym.file_id as ubigint);
                html.append_view(f_id.to_view());
                html.append_view(".html#");
                html.append_view(sym.name.to_view());
                html.append_view("'>");
                html.append_view(sym.name.to_view());
                html.append_view("</a> <small>(");
                html.append_view(get_kind_label(sym.kind));
                html.append_view(")</small></li>");
            }
        }
        html.append_view("</ul></div></div>");
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
    }

    func generate_sidebar(&mut self, rel_root : &std::string_view) : std::string {
        var html = std::string("<div class='sidebar'>");
        html.append_view("<div class='search-box'><input type='text' id='search-input' placeholder='Search...' oninput='searchSymbols()'><div id='search-results'></div></div>");
        
        html.append_view("<h2>Themes</h2><div class='theme-toggles'>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('light')\">Light</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('dark')\">Dark</button>");
        html.append_view("<button class='theme-btn' onclick=\"setTheme('paper')\">Paper</button>");
        html.append_view("</div>");

        html.append_view("<h2>Modules</h2><ul class='nav-list'>");
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
        return html;
    }

    func generate_file_docs(&mut self, file_meta : *ASTFileMetaData, mod_dir : std::string_view, mod_name : std::string_view) {
        var file_scope = file_meta.getFileScope();
        if (file_scope == null) return;

        var scope = file_scope.getBody();
        if (scope == null) return;

        var nodes = scope.getNodes();
        if (nodes == null || nodes.size() == 0) return; // Skip empty files

        var file_id = file_meta.getFileId();
        var tokens = self.ctx.getFileTokens(file_id);
        var rel_root = get_relative_root(2); // mod/file.html -> 2 levels to root

        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>");
        html.append_view(file_meta.getAbsPath());
        html.append_view("</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style></head><body><div class='layout'>");

        html.append_string(self.generate_sidebar(rel_root.to_view()));

        html.append_view("<div class='main-content'>");
        
        // Breadcrumbs
        html.append_view("<div class='breadcrumb'><a href='");
        html.append_view(rel_root.to_view());
        html.append_view("/index.html'>Home</a> / ");
        html.append_view(mod_name);
        html.append_view(" / ");
        html.append_view(file_meta.getAbsPath());
        html.append_view("</div>");

        html.append_view("<div class='header'><h1>File: ");
        html.append_view(file_meta.getAbsPath());
        html.append_view("</h1></div>");

        var i = 0u;
        while (i < nodes.size()) {
            var node = nodes.get(i);
            self.document_node(node, html, tokens, rel_root.to_view());
            i++;
        }

        html.append_view("</div></div>");
        html.append_string(self.get_js(rel_root.to_view()));
        html.append_view("</body></html>");

        var out_file = std::string();
        out_file.append_view(mod_dir);
        out_file.append_view("/");
        var file_id_str = std::string("");
        file_id_str.append_uinteger(file_id as ubigint);
        out_file.append_view(file_id_str.to_view());
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
        }
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
                    self.render_type(gen.getArgumentType(i), html, rel_root);
                }
                html.append_view("&gt;");
            }
        } else {
            html.append_view(self.get_type_name(type));
        }
    }

    func document_node(&mut self, node : *ASTNode, html : &mut std::string, tokens : std::span<Token>, rel_root : &std::string_view) {
        var name = get_node_name(node);
        if (name.size() == 0) return;

        var kind = node.getKind();
        var kind_label = get_kind_label(kind);
        var access = node.getAccessSpecifier();

        html.append_view("<div class='node' id='");
        html.append_view(name);
        html.append_view("'>");
        
        // Header
        html.append_view("<div class='node-header'>");
        if (access == AccessSpecifier.Public) html.append_view("<span class='attr-badge'>Public</span>");
        html.append_view("<span class='kind-badge'>");
        html.append_view(kind_label);
        html.append_view("</span> ");
        html.append_view("<span class='node-title'>");
        html.append_view(name);
        html.append_view("</span></div>");

        // Signature with highlighting
        var sig_raw = std::string("");
        if (kind == ASTNodeKind.FunctionDecl) {
            var decl = node as *FunctionDeclaration;
            var attrs : FuncDeclAttributesCBI = zeroed<FuncDeclAttributesCBI>();
            decl.getAttributes(&mut attrs);
            
            if (attrs.is_comptime) sig_raw.append_view("comptime ");
            if (attrs.is_extern) sig_raw.append_view("extern ");
            sig_raw.append_view("func ");
            sig_raw.append_view(name);
            sig_raw.append_view("(");
            var params = decl.get_params();
            for (var i = 0u; i < params.size(); i++) {
                if (i > 0) sig_raw.append_view(", ");
                var param = params.get(i);
                sig_raw.append_view(param.getName());
                sig_raw.append_view(" : ");
                // For signature highlighting we just want the text, we'll link later?
                // Actually easier to just append to a string and then highlight
                sig_raw.append_view("TYPE_PLACEHOLDER"); // We'll handle links manually or simplify
            }
            sig_raw.append_view(") : TYPE_RETURN");
        } else if (kind == ASTNodeKind.InterfaceDecl) {
             var def = node as *InterfaceDefinition;
             var attrs = zeroed<InterfaceDefinitionAttrsCBI>();
             def.getAttributes(&mut attrs);
             if (attrs.is_static) sig_raw.append_view("static ");
             sig_raw.append_view("interface ");
             sig_raw.append_view(name);
        } else if (kind == ASTNodeKind.NamespaceDecl) {
             sig_raw.append_view("namespace ");
             sig_raw.append_view(name);
        } else {
             sig_raw.append_view(kind_label);
             sig_raw.append_view(" ");
             sig_raw.append_view(name);
        }

        // Just use the simplified sig rendering for now but with helper
        html.append_view("<div class='signature'>");
        // Re-implementing signature bit to include links
        if (kind == ASTNodeKind.FunctionDecl) {
            var decl = node as *FunctionDeclaration;
            var attrs : FuncDeclAttributesCBI = zeroed<FuncDeclAttributesCBI>();
            decl.getAttributes(&mut attrs);
            if (attrs.is_comptime) html.append_view("<span class='tok-kwd'>comptime</span> ");
            if (attrs.is_extern) html.append_view("<span class='tok-kwd'>extern</span> ");
            html.append_view("<span class='tok-kwd'>func</span> <span class='tok-fn'>");
            html.append_view(name);
            html.append_view("</span>(");
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
        } else if (kind == ASTNodeKind.NamespaceDecl) {
            html.append_view("<span class='tok-kwd'>namespace</span> ");
            html.append_view(name);
        } else if (kind == ASTNodeKind.StructDecl) {
            html.append_view("<span class='tok-kwd'>struct</span> ");
            html.append_view(name);
        } else if (kind == ASTNodeKind.TypealiasStmt) {
            var stmt = node as *TypealiasStatement;
            html.append_view("<span class='tok-kwd'>type</span> ");
            html.append_view(name);
            html.append_view(" = ");
            self.render_type(stmt.getActualType(), html, rel_root);
        }
        html.append_view("</div>");

        // Comment
        var encoded_loc = node.getEncodedLocation();
        var loc_data = self.ctx.decodeLocation(encoded_loc);
        var comment = find_comment_before(tokens, loc_data.lineStart);
        if (comment.size() > 0) {
            html.append_view("<div class='comment'>");
            html.append_view(highlight_chemical(self.clean_comment(comment).to_view()).to_view());
            html.append_view("</div>");
        }

        // Recursion for members
        if (kind == ASTNodeKind.NamespaceDecl) {
            var ns = node as *Namespace;
            var children = ns.get_body();
            if (children != null && children.size() > 0) {
                html.append_view("<div class='nested' style='margin-left: 2rem; border-left: 2px solid var(--border); padding-left: 1rem;'>");
                for (var i = 0u; i < children.size(); i++) {
                    self.document_node(children.get(i), html, tokens, rel_root);
                }
                html.append_view("</div>");
            }
        } else if (kind == ASTNodeKind.StructDecl) {
            var def = node as *StructDefinition;
            var funcs = def.getFunctions();
            if (funcs != null && funcs.size() > 0) {
                html.append_view("<div class='nested' style='margin-left: 2rem; border-left: 2px solid var(--border); padding-left: 1rem;'>");
                for (var i = 0u; i < funcs.size(); i++) {
                    self.document_node(funcs.get(i), html, tokens, rel_root);
                }
                html.append_view("</div>");
            }
            // TODO: document members too
        }

        html.append_view("</div>");
    }

    func clean_comment(&self, comment : std::string_view) : std::string {
        // TODO: remove leading asterisks for block comments
        return std::string(comment.data(), comment.size());
    }

    func generate_index_html(&mut self) {
        var rel_root = std::string(".");
        var html = std::string("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
        html.append_view("<title>API Documentation</title><style>");
        html.append_view(self.get_css());
        html.append_view("</style></head><body><div class='layout'>");
        
        html.append_string(self.generate_sidebar(rel_root.to_view()));

        html.append_view("<div class='main-content'>");
        html.append_view("<div class='header'><h1>API Documentation</h1></div>");
        html.append_view("<h2>Welcome to the Chemical API reference.</h2>");
        html.append_view("<p>Use the sidebar to explore modules or the search bar to find specific symbols.</p>");
        
        html.append_view("<h3>Available Modules</h3><ul class='nav-list'>");
        var last_mod = std::string("");
        for (var i = 0u; i < self.index.size(); i++) {
            var sym = self.index.get_ptr(i);
            if (!sym.mod_name.equals(last_mod)) {
                html.append_view("<li><a href='./");
                html.append_view(sym.mod_name.to_view());
                html.append_view("/index.html' style='font-size: 1.2rem; font-weight: 600;'>");
                html.append_view(sym.mod_name.to_view());
                html.append_view("</a></li>");
                last_mod = sym.mod_name.copy();
            }
        }
        html.append_view("</ul></div></div>");
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

    func get_js(&self, rel_root : &std::string_view) : std::string {
        var s = std::string("<script src='");
        s.append_view(rel_root);
        s.append_view("/search_index.js'></script>");
        s.append_view("""
            <script>
            function setTheme(theme) {
                document.documentElement.setAttribute('data-theme', theme);
                localStorage.setItem('refgen-theme', theme);
            }
            const savedTheme = localStorage.getItem('refgen-theme') || 'paper';
            document.documentElement.setAttribute('data-theme', savedTheme);

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
            </script>
        """);
        // Replace REL_ROOT_VAR with actual rel_root
        var res = std::string("");
        var raw = s.to_view();
        var placeholder = std::string_view("${REL_ROOT_VAR}");
        var last_idx = 0u;
        while (true) {
            var p_idx = raw.subview(last_idx, raw.size()).find(placeholder);
            if (p_idx == -1u) {
                res.append_view(raw.subview(last_idx, raw.size()));
                break;
            }
            res.append_view(raw.subview(last_idx, p_idx - last_idx));
            res.append_view(rel_root);
            last_idx = p_idx + placeholder.size();
        }
        return res;
    }

    func get_css(&self) : std::string_view {
        return """
            :root {
                --transition: 0.2s cubic-bezier(0.4, 0, 0.2, 1);
            }
            :root[data-theme='light'] {
                --bg: #ffffff; --bg-card: #f9fafb; --border: #e5e7eb; --text: #111827; --text-muted: #4b5563; --accent: #2563eb; --code-bg: #f3f4f6;
            }
            :root[data-theme='dark'] {
                --bg: #030712; --bg-card: #0f172a; --border: #1e293b; --text: #f8fafc; --text-muted: #94a3b8; --accent: #38bdf8; --code-bg: #1e293b;
            }
            :root[data-theme='paper'] {
                --bg: #f4f1ea; --bg-card: #fdfcf9; --border: #e2ddd3; --text: #433f38; --text-muted: #7c7467; --accent: #8b5e34; --code-bg: #e9e4d9;
            }
            body { 
                font-family: 'Outfit', 'Inter', system-ui, sans-serif; 
                background: var(--bg); color: var(--text); padding: 0; margin: 0; line-height: 1.6;
                transition: background var(--transition), color var(--transition);
            }
            .layout { display: flex; min-height: 100vh; }
            .sidebar { 
                width: 300px; background: var(--bg-card); border-right: 1px solid var(--border); 
                padding: 2rem 1.5rem; position: sticky; top: 0; height: 100vh; overflow-y: auto;
            }
            .main-content { flex: 1; padding: 2rem 4rem; max-width: 900px; }
            .header { 
                display: flex; justify-content: space-between; align-items: center; margin-bottom: 2rem;
                padding-bottom: 1rem; border-bottom: 2px solid var(--border);
            }
            .search-box { position: relative; margin-bottom: 2rem; }
            #search-input { 
                width: 100%; padding: 0.75rem 1rem; border: 1px solid var(--border); border-radius: 8px;
                background: var(--bg); color: var(--text); outline: none;
            }
            #search-results {
                position: absolute; top: 100%; left: 0; right: 0; background: var(--bg-card);
                border: 1px solid var(--border); border-radius: 8px; display: none; z-index: 1000;
                box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1);
            }
            #search-results div { padding: 0.75rem 1rem; border-bottom: 1px solid var(--border); }
            #search-results a { display: block; }
            .node { 
                background: var(--bg-card); border: 1px solid var(--border); padding: 2rem; 
                margin-bottom: 3rem; border-radius: 12px; box-shadow: 0 1px 3px rgba(0,0,0,0.05);
            }
            .kind-badge { 
                background: var(--accent); color: white; padding: 2px 10px; border-radius: 4px; 
                font-size: 0.7rem; font-weight: 700; text-transform: uppercase; margin-right: 0.5rem;
            }
            .attr-badge { border: 1px solid var(--accent); color: var(--accent); padding: 1px 6px; border-radius: 4px; font-size: 0.7rem; margin-right: 4px; }
            .node-title { font-size: 2rem; font-weight: 800; margin: 0.5rem 0; }
            .signature { 
                font-family: 'JetBrains Mono', monospace; background: var(--code-bg); padding: 1rem;
                border-radius: 8px; margin: 1.5rem 0; font-size: 0.95rem; border: 1px solid var(--border);
            }
            .tok-kwd { color: #d73a49; font-weight: 600; }
            .tok-type { color: #6f42c1; }
            .tok-str { color: #032f62; }
            .tok-com { color: var(--text-muted); font-style: italic; }
            .comment { color: var(--text); font-size: 1.05rem; margin-top: 1.5rem; }
            .theme-toggles { display: flex; gap: 0.5rem; }
            .theme-btn { 
                cursor: pointer; padding: 4px 8px; border: 1px solid var(--border); border-radius: 4px; background: var(--bg-card);
            }
            a { color: var(--accent); text-decoration: none; }
            a:hover { text-decoration: underline; }
            .nav-list { list-style: none; padding: 0; }
            .nav-list li { margin-bottom: 0.5rem; }
            .breadcrumb { font-size: 0.9rem; color: var(--text-muted); margin-bottom: 1rem; }
        """;
    }
}

}
