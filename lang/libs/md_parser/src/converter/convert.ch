/**
 * Shared MD AST → HTML text conversion.
 *
 * Extracted from md_cbi and md runtime to eliminate duplication.
 * The emitter handles text output; code highlighting and link rewriting
 * are passed as function pointers.
 */

using namespace std;

public func md_escape_html(str : &mut std::string, text : std::string_view) {
    var i = 0u
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF
        if (c1 < 0x80) {
            const c = c1 as char
            switch(c) {
                '<' => { str.append_view("&lt;") }
                '>' => { str.append_view("&gt;") }
                '&' => { str.append_view("&amp;") }
                '"' => { str.append_view("&quot;") }
                default => { str.append(c) }
            }
            i++
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F)
                str.append_view("&#"); str.append_integer(codepoint as bigint); str.append(';')
                i += 2
            } else { i++ }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)
                str.append_view("&#"); str.append_integer(codepoint as bigint); str.append(';')
                i += 3
            } else { i++ }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF
                const c3 = (text.data()[i+2] as uint) & 0xFF
                const c4 = (text.data()[i+3] as uint) & 0xFF
                const codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F)
                str.append_view("&#"); str.append_integer(codepoint as bigint); str.append(';')
                i += 4
            } else { i++ }
        } else { i++ }
    }
}

public func md_get_align_style(align : MdTableAlign) : std::string_view {
    switch(align) {
        MdTableAlign.Left => { return std::string_view(" style=\"text-align:left\"") }
        MdTableAlign.Center => { return std::string_view(" style=\"text-align:center\"") }
        MdTableAlign.Right => { return std::string_view(" style=\"text-align:right\"") }
        default => { return std::string_view("") }
    }
}

public func md_convert_children(str : &mut std::string, children : &std::vector<*mut MdNode>, emitter : *mut MdEmitter, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) {
    var i = 0u
    while(i < children.size()) {
        md_convert_md_node(str, children.get(i), emitter, highlighter, link_rewriter)
        i++
    }
}

public func md_convert_md_node(str : &mut std::string, node : *mut MdNode, emitter : *mut MdEmitter, highlighter : (lang : std::string_view, code : std::string_view) => std::string, link_rewriter : (url : std::string_view) => std::string) {
    if(node == null) return
    switch(node.kind) {
        MdNodeKind.Root => {
            var root = node as *mut MdRoot
            md_convert_children(str, &root.children, emitter, highlighter, link_rewriter)
        }
        MdNodeKind.Header => {
            var header = node as *mut MdHeader
            str.append_view("<h")
            str.append_integer(header.level as bigint)
            str.append_view(" class=\"md-hg md-h")
            str.append_integer(header.level as bigint)
            str.append_view("\">")
            md_convert_children(str, &header.children, emitter, highlighter, link_rewriter)
            str.append_view("</h")
            str.append_integer(header.level as bigint)
            str.append_view(">\n")
        }
        MdNodeKind.Paragraph => {
            var para = node as *mut MdParagraph
            str.append_view("<p class=\"md-p\">")
            md_convert_children(str, &para.children, emitter, highlighter, link_rewriter)
            str.append_view("</p>\n")
        }
        MdNodeKind.Bold => {
            var bold = node as *mut MdBold
            str.append_view("<strong class=\"md-bold\">")
            md_convert_children(str, &bold.children, emitter, highlighter, link_rewriter)
            str.append_view("</strong>")
        }
        MdNodeKind.Italic => {
            var italic = node as *mut MdItalic
            str.append_view("<em class=\"md-italic\">")
            md_convert_children(str, &italic.children, emitter, highlighter, link_rewriter)
            str.append_view("</em>")
        }
        MdNodeKind.Strikethrough => {
            var strike = node as *mut MdStrikethrough
            str.append_view("<del class=\"md-del\">")
            md_convert_children(str, &strike.children, emitter, highlighter, link_rewriter)
            str.append_view("</del>")
        }
        MdNodeKind.Link => {
            var link = node as *mut MdLink
            str.append_view("<a class=\"md-link\" href=\"")
            if(link_rewriter != null) {
                var rewritten = link_rewriter(link.url)
                str.append_view(rewritten.to_view())
            } else {
                str.append_view(&link.url)
            }
            str.append_view("\"")
            if(link.title.size() > 0) {
                str.append_view(" title=\"")
                md_escape_html(str, link.title)
                str.append_view("\"")
            }
            str.append_view(">")
            md_convert_children(str, &link.children, emitter, highlighter, link_rewriter)
            str.append_view("</a>")
        }
        MdNodeKind.AutoLink => {
            var autolink = node as *mut MdAutoLink
            str.append_view("<a class=\"md-link md-autolink\" href=\"")
            str.append_view(&autolink.url)
            str.append_view("\">")
            str.append_view(&autolink.url)
            str.append_view("</a>")
        }
        MdNodeKind.Image => {
            var img = node as *mut MdImage
            str.append_view("<img class=\"md-img\" src=\"")
            str.append_view(&img.url)
            str.append_view("\" alt=\"")
            md_escape_html(str, img.alt)
            str.append_view("\"")
            if(img.title.size() > 0) {
                str.append_view(" title=\"")
                md_escape_html(str, img.title)
                str.append_view("\"")
            }
            str.append_view("/>")
        }
        MdNodeKind.InlineCode => {
            var code = node as *mut MdInlineCode
            str.append_view("<code class=\"md-code\">")
            md_escape_html(str, code.value)
            str.append_view("</code>")
        }
        MdNodeKind.CodeBlock => {
            var cb = node as *mut MdCodeBlock
            str.append_view("<pre class=\"md-pre\"><code class=\"md-code-block")
            if(cb.language.size() > 0) {
                str.append_view(" language-")
                str.append_view(&cb.language)
            }
            str.append_view("\">")
            if(highlighter != null) {
                var res = highlighter(cb.language, cb.code)
                if(res.size() > 0) {
                    str.append_view(res.to_view())
                } else {
                    md_escape_html(str, cb.code)
                }
            } else {
                md_escape_html(str, cb.code)
            }
            str.append_view("</code></pre>\n")
        }
        MdNodeKind.Blockquote => {
            var bq = node as *mut MdBlockquote
            if (bq.alert_type.size() > 0) {
                str.append_view("<blockquote class=\"md-blockquote md-alert md-alert-")
                var i = 0u
                while (i < bq.alert_type.size()) {
                    const c = bq.alert_type.data()[i]
                    if (c >= 'A' && c <= 'Z') { str.append((c as int + 32) as char) } else { str.append(c) }
                    i++
                }
                str.append_view("\">\n<div class=\"md-alert-title\">")
                str.append_view(&bq.alert_type)
                str.append_view("</div>")
            } else {
                str.append_view("<blockquote class=\"md-blockquote\">")
            }
            md_convert_children(str, &bq.children, emitter, highlighter, link_rewriter)
            str.append_view("</blockquote>\n")
        }
        MdNodeKind.Hr => { str.append_view("<hr class=\"md-hr\"/>\n") }
        MdNodeKind.List => {
            var list = node as *mut MdList
            if(list.ordered) {
                str.append_view("<ol class=\"md-ol\"")
                if(list.start != 1) { str.append_view(" start=\""); str.append_integer(list.start as bigint); str.append_view("\"") }
                str.append_view(">")
            } else { str.append_view("<ul class=\"md-ul\">") }
            md_convert_children(str, &list.children, emitter, highlighter, link_rewriter)
            if(list.ordered) { str.append_view("</ol>\n") } else { str.append_view("</ul>\n") }
        }
        MdNodeKind.ListItem => {
            var item = node as *mut MdListItem
            str.append_view("<li class=\"md-li\">")
            md_convert_children(str, &item.children, emitter, highlighter, link_rewriter)
            str.append_view("</li>\n")
        }
        MdNodeKind.Table => {
            var table = node as *mut MdTable
            str.append_view("<table class=\"md-table\">\n")
            var row_idx = 0u
            while(row_idx < table.children.size()) {
                const row_node = table.children.get(row_idx)
                const row = row_node as *mut MdTableRow
                if(row.is_header) {
                    str.append_view("<thead class=\"md-thead\">\n<tr class=\"md-tr\">")
                    var col_idx = 0u
                    while(col_idx < row.children.size()) {
                        const cell = row.children.get(col_idx) as *mut MdTableCell
                        str.append_view("<th class=\"md-th\"")
                        if(col_idx < table.alignments.size()) { str.append_view(md_get_align_style(table.alignments.get(col_idx))) }
                        str.append_view(">")
                        md_convert_children(str, &cell.children, emitter, highlighter, link_rewriter)
                        str.append_view("</th>")
                        col_idx++
                    }
                    str.append_view("</tr>\n</thead>\n<tbody class=\"md-tbody\">\n")
                } else {
                    str.append_view("<tr class=\"md-tr\">")
                    var col_idx = 0u
                    while(col_idx < row.children.size()) {
                        const cell = row.children.get(col_idx) as *mut MdTableCell
                        str.append_view("<td class=\"md-td\"")
                        if(col_idx < table.alignments.size()) { str.append_view(md_get_align_style(table.alignments.get(col_idx))) }
                        str.append_view(">")
                        md_convert_children(str, &cell.children, emitter, highlighter, link_rewriter)
                        str.append_view("</td>")
                        col_idx++
                    }
                    str.append_view("</tr>\n")
                }
                row_idx++
            }
            str.append_view("</tbody>\n</table>\n")
        }
        MdNodeKind.TableRow => {}
        MdNodeKind.TableCell => {}
        MdNodeKind.Text => {
            var text = node as *mut MdText
            md_escape_html(str, text.value)
        }
        MdNodeKind.Interpolation => {
            emitter.flush()
            var interp = node as *mut MdInterpolation
            emitter.emit_interpolation(interp.value)
        }
        MdNodeKind.Superscript => {
            var sup = node as *mut MdSuperscript
            str.append_view("<sup class=\"md-sup\">")
            md_convert_children(str, &sup.children, emitter, highlighter, link_rewriter)
            str.append_view("</sup>")
        }
        MdNodeKind.Subscript => {
            var sub = node as *mut MdSubscript
            str.append_view("<sub class=\"md-sub\">")
            md_convert_children(str, &sub.children, emitter, highlighter, link_rewriter)
            str.append_view("</sub>")
        }
        MdNodeKind.Insert => {
            var ins = node as *mut MdInsert
            str.append_view("<ins class=\"md-ins\">")
            md_convert_children(str, &ins.children, emitter, highlighter, link_rewriter)
            str.append_view("</ins>")
        }
        MdNodeKind.Mark => {
            var mark = node as *mut MdMark
            str.append_view("<mark class=\"md-mark\">")
            md_convert_children(str, &mark.children, emitter, highlighter, link_rewriter)
            str.append_view("</mark>")
        }
        MdNodeKind.Footnote => {
            var fn = node as *mut MdFootnote
            str.append_view("<sup class=\"md-footnote-ref\" id=\"fnref:")
            str.append_view(&fn.id)
            str.append_view("\"><a href=\"#fn:")
            str.append_view(&fn.id)
            str.append_view("\">")
            md_escape_html(str, fn.id)
            str.append_view("</a></sup>")
        }
        MdNodeKind.FootnoteDef => {
            var fd = node as *mut MdFootnoteDef
            str.append_view("<div class=\"md-footnote-def\" id=\"fn:")
            str.append_view(&fd.id)
            str.append_view("\"><span class=\"md-footnote-id\">")
            str.append_view(&fd.id)
            str.append_view(": </span>")
            md_convert_children(str, &fd.children, emitter, highlighter, link_rewriter)
            str.append_view("</div>\n")
        }
        MdNodeKind.DefinitionList => {
            var dl = node as *mut MdDefinitionList
            str.append_view("<dl class=\"md-dl\">\n")
            md_convert_children(str, &dl.children, emitter, highlighter, link_rewriter)
            str.append_view("</dl>\n")
        }
        MdNodeKind.DefinitionTerm => {
            var dt = node as *mut MdDefinitionTerm
            str.append_view("<dt class=\"md-dt\">")
            md_convert_children(str, &dt.children, emitter, highlighter, link_rewriter)
            str.append_view("</dt>\n")
        }
        MdNodeKind.DefinitionData => {
            var dd = node as *mut MdDefinitionData
            str.append_view("<dd class=\"md-dd\">")
            md_convert_children(str, &dd.children, emitter, highlighter, link_rewriter)
            str.append_view("</dd>\n")
        }
        MdNodeKind.Abbreviation => {
            var abb = node as *mut MdAbbreviation
            str.append_view("<abbr title=\"")
            md_escape_html(str, abb.title)
            str.append_view("\">")
            md_escape_html(str, abb.id)
            str.append_view("</abbr>")
        }
        MdNodeKind.CustomContainer => {
            var cc = node as *mut MdCustomContainer
            str.append_view("<div class=\"md-container md-")
            str.append_view(&cc.type)
            str.append_view("\">\n")
            md_convert_children(str, &cc.children, emitter, highlighter, link_rewriter)
            str.append_view("</div>\n")
        }
        MdNodeKind.TaskCheckbox => {
            var cb = node as *mut MdTaskCheckbox
            str.append_view("<input class=\"md-task-checkbox\" type=\"checkbox\" disabled")
            if(cb.checked) { str.append_view(" checked") }
            str.append_view("/>")
        }
    }
}
