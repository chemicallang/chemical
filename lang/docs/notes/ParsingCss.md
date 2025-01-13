### Operators in CSS

Colon (:)
Semicolon (;)
Curly Braces ({ })
Comma (,)
Slash (/)
Addition (+)
Subtraction (-)
Multiplication (*)
Division (/)
Equal to (=)
Greater than (>)
Less than (<)
Greater than or equal to (>=)
Less than or equal to (<=)
Exact match (=)
Contains word (~=)
Contains substring (*=)
Starts with (^=)
Ends with ($=)
Dash-separated match (|=)

#### Combinators
Child (>)
Adjacent Sibling (+)
General Sibling (~)

### Other Syntax Stuff
Double Quotes (")
Single Quotes (')
Block Comment (/* ... */)


### Keywords in CSS

#### Named Colors
Common examples include: red, green, blue, yellow, cyan, magenta, black, white, gray, etc. (Refer to the CSS color list for the full set of 140+ named colors.)

The full set of color keywords are available at

https://www.w3.org/wiki/CSS/Properties/color/keywords

#### General Syntax Keywords

@charset
@import
@namespace
@media
@supports
@keyframes
@font-face
@page
@counter-style
@property
!important
Commonly Used Property Value Keywords
Color Keywords
transparent
currentColor

#### Display Keywords

block
inline
inline-block
flex
grid
inline-flex
inline-grid
none
table
table-cell
table-row
list-item
run-in

#### Positioning Keywords
static
relative
absolute
fixed
sticky

#### Overflow Keywords
visible
hidden
scroll
auto

### Visibility Keywords
visible
hidden
collapse

### Flexbox and Grid Keywords

#### Alignment Keywords
flex-start
flex-end
center
space-between
space-around
space-evenly
start
end
baseline
stretch

#### Grid Keywords
auto
min-content
max-content
fit-content
repeat
span

### Font Keywords
serif
sans-serif
monospace
cursive
fantasy
system-ui
inherit
initial
unset

#### Font Style Keywords
normal
italic
oblique

#### Font Weight Keywords
normal
bold
lighter
bolder

#### Background Keywords
cover
contain
no-repeat
repeat
repeat-x
repeat-y
space
round

### Border and Outline Keywords
#### Border Style Keywords
none
solid
dotted
dashed
double
groove
ridge
inset
outset
hidden

### Animation Keywords
linear
ease
ease-in
ease-out
ease-in-out
infinite
alternate
reverse
forwards
backwards

### Sizing Keywords
auto
contain
cover
stretch

### CSS Global Values
inherit
initial
unset



Here is a comprehensive list of CSS keywords that you’ll need to account for when writing a CSS lexer. These keywords are categorized based on their use in CSS syntax.

General Syntax Keywords
@charset
@import
@namespace
@media
@supports
@keyframes
@font-face
@page
@counter-style
@property
!important
Commonly Used Property Value Keywords
Color Keywords
transparent
currentColor
Named Colors
Common examples include: red, green, blue, yellow, cyan, magenta, black, white, gray, etc. (Refer to the CSS color list for the full set of 140+ named colors.)

Display Keywords
block
inline
inline-block
flex
grid
inline-flex
inline-grid
none
table
table-cell
table-row
list-item
run-in
Positioning Keywords
static
relative
absolute
fixed
sticky
Overflow Keywords
visible
hidden
scroll
auto
Visibility Keywords
visible
hidden
collapse
Flexbox and Grid Keywords
Alignment Keywords
flex-start
flex-end
center
space-between
space-around
space-evenly
start
end
baseline
stretch
Grid Keywords
auto
min-content
max-content
fit-content
repeat
span
Font Keywords
serif
sans-serif
monospace
cursive
fantasy
system-ui
inherit
initial
unset
Font Style Keywords
normal
italic
oblique
Font Weight Keywords
normal
bold
lighter
bolder
Numeric values: 100 to 900
Background Keywords
cover
contain
no-repeat
repeat
repeat-x
repeat-y
space
round
Border and Outline Keywords
Border Style Keywords
none
solid
dotted
dashed
double
groove
ridge
inset
outset
hidden
Animation Keywords
linear
ease
ease-in
ease-out
ease-in-out
infinite
alternate
reverse
forwards
backwards
Sizing Keywords
auto
contain
cover
stretch
CSS Global Values
inherit
initial
unset

### Unit Keywords
Units must be treated as keywords in your lexer to distinguish them from numbers:
Length Units: px, em, rem, vh, vw, vmin, vmax, %, cm, mm, in, pt, pc, ch, ex
Time Units: s, ms
Frequency Units: Hz, kHz
Angle Units: deg, rad, grad, turn

### Functional Keywords
CSS has functions that act as keywords:
rgb()
rgba()
hsl()
hsla()
calc()
var()
clamp()
min()
max()
url()
attr()
env()
cubic-bezier()
steps()

## Pseudo-Classes and Pseudo-Elements
Pseudo-classes and pseudo-elements start with : or ::. Your lexer must recognize these separately.

### Pseudo-Classes (e.g., :hover, :nth-child())
:active
:checked
:disabled
:empty
:enabled
:first-child
:first-of-type
:focus
:hover
:in-range
:invalid
:last-child
:last-of-type
:link
:not()
:nth-child()
:nth-last-child()
:nth-of-type()
:nth-last-of-type()
:only-child
:only-of-type
:optional
:out-of-range
:placeholder-shown
:read-only
:read-write
:required
:root
:target
:valid
:visited

## Pseudo-Elements (e.g., ::before, ::after)
::before
::after
::first-line
::first-letter
::placeholder
::marker


### Special Properties

These properties require special parsing logic

#### Special Properties (suggested by ChatGPT)
background
border
border-radius
margin
padding
font
flex
grid
grid-template
animation
transition
box-shadow
text-shadow
clip-path
mask
list-style
outline
place-items	[ <align-items> <justify-items> ]	Defines alignment for grid or flex container items.
align-items
gap

### Special Properties (suggested from the libcss repository)
align_content
align_items
align_self
azimuth
background
background_attachment
background_color
background_image
background_position
background_repeat
border
border_bottom
border_bottom_color
border_bottom_style
border_bottom_width
border_collapse
border_color
border_left
border_left_color
border_left_style
border_left_width
border_right
border_right_color
border_right_style
border_right_width
border_spacing
border_style
border_top
border_top_color
border_top_style
border_top_width
border_width
bottom
box_sizing
break_after
break_before
break_inside
caption_side
clear
clip
color
columns
column_count
column_fill
column_gap
column_rule
column_rule_color
column_rule_style
column_rule_width
column_span
column_width
content
counter_increment
counter_reset
cue
cue_after
cue_before
cursor
direction
display
elevation
empty_cells
fill_opacity
flex
flex_basis
flex_direction
flex_flow
flex_grow
flex_shrink
flex_wrap
float
font
font_family
font_size
font_style
font_variant
font_weight
height
justify_content
left
letter_spacing
line_height
list_style
list_style_image
list_style_position
list_style_type
margin
margin_bottom
margin_left
margin_right
margin_top
max_height
max_width
min_height
min_width
opacity
order
orphans
outline
outline_color
outline_style
outline_width
overflow
overflow_x
overflow_y
padding
padding_bottom
padding_left
padding_right
padding_top
page_break_after
page_break_before
page_break_inside
pause
pause_after
pause_before
pitch_range
pitch
play_during
position
quotes
richness
right
speak_header
speak_numeral
speak_punctuation
speak
speech_rate
stress
stroke_opacity
table_layout
text_align
text_decoration
text_indent
text_transform
top
unicode_bidi
vertical_align
visibility
voice_family
volume
white_space
widows
width
word_spacing
writing_mode
z_index

AZIMUTH
BACKGROUND_ATTACHMENT
BACKGROUND_COLOR
BACKGROUND_IMAGE
BACKGROUND_POSITION
BACKGROUND_REPEAT
BORDER_COLLAPSE
BORDER_SPACING
BORDER_TOP_COLOR
BORDER_RIGHT_COLOR
BORDER_BOTTOM_COLOR
BORDER_LEFT_COLOR
BORDER_TOP_STYLE
BORDER_RIGHT_STYLE
BORDER_BOTTOM_STYLE
BORDER_LEFT_STYLE
BORDER_TOP_WIDTH
BORDER_RIGHT_WIDTH
BORDER_BOTTOM_WIDTH
BORDER_LEFT_WIDTH
BOTTOM
CAPTION_SIDE
CLEAR
CLIP
COLOR
CONTENT
COUNTER_INCREMENT
COUNTER_RESET
CUE_AFTER
CUE_BEFORE
CURSOR
DIRECTION
DISPLAY
ELEVATION
EMPTY_CELLS
FLOAT
FONT_FAMILY
FONT_SIZE
FONT_STYLE
FONT_VARIANT
FONT_WEIGHT
HEIGHT
LEFT
LETTER_SPACING
LINE_HEIGHT
LIST_STYLE_IMAGE
LIST_STYLE_POSITION
LIST_STYLE_TYPE
MARGIN_TOP
MARGIN_RIGHT
MARGIN_BOTTOM
MARGIN_LEFT
MAX_HEIGHT
MAX_WIDTH
MIN_HEIGHT
MIN_WIDTH
ORPHANS
OUTLINE_COLOR
OUTLINE_STYLE
OUTLINE_WIDTH
OVERFLOW_X
PADDING_TOP
PADDING_RIGHT
PADDING_BOTTOM
PADDING_LEFT
PAGE_BREAK_AFTER
PAGE_BREAK_BEFORE
PAGE_BREAK_INSIDE
PAUSE_AFTER
PAUSE_BEFORE
PITCH_RANGE
PITCH
PLAY_DURING
POSITION
QUOTES
RICHNESS
RIGHT
SPEAK_HEADER
SPEAK_NUMERAL
SPEAK_PUNCTUATION
SPEAK
SPEECH_RATE
STRESS
TABLE_LAYOUT
TEXT_ALIGN
TEXT_DECORATION
TEXT_INDENT
TEXT_TRANSFORM
TOP
UNICODE_BIDI
VERTICAL_ALIGN
VISIBILITY
VOICE_FAMILY
VOLUME
WHITE_SPACE
WIDOWS
WIDTH
WORD_SPACING
Z_INDEX
OPACITY
FILL_OPACITY
STROKE_OPACITY
BREAK_AFTER
BREAK_BEFORE
BREAK_INSIDE
COLUMN_COUNT
COLUMN_FILL
COLUMN_GAP
COLUMN_RULE_COLOR
COLUMN_RULE_STYLE
COLUMN_RULE_WIDTH
COLUMN_SPAN
COLUMN_WIDTH
WRITING_MODE
OVERFLOW_Y
BOX_SIZING
ALIGN_CONTENT
ALIGN_ITEMS
ALIGN_SELF
FLEX_BASIS
FLEX_DIRECTION
FLEX_GROW
FLEX_SHRINK
FLEX_WRAP
JUSTIFY_CONTENT
ORDER,