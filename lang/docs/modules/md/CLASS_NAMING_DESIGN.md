# Markdown Class Naming Design Plan

## Overview
This document defines the consistent class naming scheme for the Chemical Markdown renderer. All class names follow the `md-*` prefix pattern to ensure they don't conflict with user CSS and can be easily targeted by themes.

## Current Class Names (from themes)
Based on existing theme files, these class names are already implemented and must be preserved:

### Block Elements
- `md-p` - Paragraphs
- `md-hg` - Header group (base class for all headers)
- `md-h1`, `md-h2`, `md-h3`, `md-h4`, `md-h5`, `md-h6` - Individual header levels
- `md-blockquote` - Blockquotes
- `md-hr` - Horizontal rules
- `md-pre` - Preformatted text containers
- `md-code-block` - Code blocks (inside pre)
- `md-table` - Tables
- `md-thead` - Table header section
- `md-tbody` - Table body section
- `md-tr` - Table rows
- `md-th` - Table header cells
- `md-td` - Table data cells
- `md-ul` - Unordered lists
- `md-ol` - Ordered lists
- `md-li` - List items
- `md-container` - Custom containers (with type-specific modifier)

### Inline Elements
- `md-link` - Links
- `md-img` - Images
- `md-code` - Inline code
- `md-bold` - Bold text (strong tag)
- `md-italic` - Italic text (em tag)
- `md-del` - Strikethrough text (del tag)
- `md-mark` - Marked text (mark tag)
- `md-ins` - Inserted text (ins tag)
- `md-sup` - Superscript
- `md-sub` - Subscript

### Special Elements (to be implemented)
- `md-task-checkbox` - Task list checkboxes
- `md-dl` - Definition lists
- `md-dt` - Definition terms
- `md-dd` - Definition descriptions
- `md-footnote-ref` - Footnote references
- `md-footnote-def` - Footnote definitions
- `md-footnote-id` - Footnote identifiers
- `md-abbr` - Abbreviations

## Class Naming Rules

### 1. Prefix Convention
All classes must use the `md-` prefix to avoid conflicts and maintain namespace consistency.

### 2. Naming Patterns
- **Block elements**: Use semantic HTML tag names (e.g., `md-p`, `md-h1`, `md-blockquote`)
- **Inline elements**: Use semantic names (e.g., `md-link`, `md-code`, `md-bold`)
- **Compound elements**: Use hyphen separation (e.g., `md-task-checkbox`, `md-footnote-ref`)
- **Modifiers**: Use additional classes for variants (e.g., `md-container md-info`)

### 3. Consistency with md_cbi
The md module should match the class names used in md_cbi module exactly. Both modules should generate identical HTML output with identical class names.

### 4. Theme Compatibility
All class names must be compatible with existing themes in `lang/compiled/md-themes/src/md-themes.ch`. No existing class names should be changed.

## Implementation Guidelines

### Table Classes
```html
<table class="md-table">
  <thead class="md-thead">
    <tr class="md-tr">
      <th class="md-th">Header</th>
    </tr>
  </thead>
  <tbody class="md-tbody">
    <tr class="md-tr">
      <td class="md-td">Data</td>
    </tr>
  </tbody>
</table>
```

### Task List Classes
```html
<ul class="md-ul">
  <li class="md-li">
    <input class="md-task-checkbox" type="checkbox" disabled checked/>
    Task text
  </li>
</ul>
```

### Definition List Classes
```html
<dl class="md-dl">
  <dt class="md-dt">Term</dt>
  <dd class="md-dd">Definition</dd>
</dl>
```

<p class="md-p">
  Text with footnote<sup class="md-footnote-ref" id="md-fnref:1">
    <a href="#md-fn:1">1</a>
  </sup>
</p>
<div class="md-footnote-def" id="md-fn:1">
  <span class="md-footnote-id">1: </span>
  <p class="md-p">Footnote content</p>
</div>
```

### Abbreviation Classes
```html
<abbr class="md-abbr" title="HyperText Markup Language">HTML</abbr>
```

## Migration Strategy

1. **Preserve existing classes**: Never change class names that are already targeted by themes
2. **Add missing classes**: Implement missing class names for unsupported features
3. **Match md_cbi output**: Ensure identical HTML output between md and md_cbi modules
4. **Test theme compatibility**: Verify all existing themes work with new classes

## Testing Requirements

- All existing tests must pass
- New tests for tables, task lists, definition lists, footnotes, and abbreviations
- Visual testing with all existing themes
- Output comparison between md and md_cbi modules
