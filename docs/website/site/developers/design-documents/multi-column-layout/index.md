---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: multi-column-layout
title: Multi-column layout
---

Spec: <http://www.w3.org/TR/css3-multicol/>

Editor's draft: <http://dev.w3.org/csswg/css-multicol/>

## The flow thread (LayoutMultiColumnFlowThread)

Breaking content into multiple columns means that line boxes may no longer only
be stacked below each other, and also that child blocks may no longer be
rectangular, visually, if they cross a column boundary. We try to worry as
little about that internally, though. A multi-column container will insert a
special child renderer - the flow thread - which lays out everything as a long
single strip (with the width of one column - all columns have the same width).
It's only during painting and hit testing (and similar operations) that we need
to break this single strip into columns.

All DOM children of the multicol container are moved into the flow thread,
instead of being direct children of the multicol container. The flow thread does
not take up any space in its container. We create LayoutMultiColumnSet siblings
(of the flow thread) that make sure that we take up the correct amount of space.
The flow thread cannot be painted / hit-tested / etc. directly.
LayoutMultiColumnSet is needed to calculate correct translation and clipping for
each column. It's needed to convert between flow thread coordinates and visual
coordinates. A LayoutMultiColumnSet represents one or more rows of columns. It
has no child layout objects. A column set contains an array of
MultiColumnFragmentainerGroup objects. There is one object per column row. We
need multiple rows when a multicol container is nested inside another
FragmentationContext (another multicol container (LayoutMultiColumnFlowThread),
or, in case of printing, pagination context (ViewFragmentationContext)).

## Definitions

So in multicol (and all other fragmentation contexts) we need to distinguish
between flow thread coordinates and visual coordinates.

Flow thread coordinates: Used internally in the layout engine. All flow thread
descendants' dimensions (and overflow rectangles, and pretty much everything
else) are in flow thread coordinates. The flow thread (which holds all multicol
content) is laid out as one tall single-column strip, not worrying much \[\*\]
about where each column begins and ends. This allows the layout engine to be
rather oblivious to fragmentation, so that e.g. a block can always be
represented as a simple rectangle internally. Each flow thread establishes their
own coordinate space, and they can be nested (multicol inside multicol).

Visual coordinates: The actual dimensions for painting, hit-testing, etc
(basically, what you see on the screen). Calculated from flow thread
coordinates, by determining which column a given point is in, and then adding
that column's offset to the inline-direction coordinate, and subtracting that
column's flow thread portion rectangle's logical top from the block-direction
coordinate.

Flow thread portion rectangle (in flow thread coordinates): Each column has a
flow thread portion rectangle. It defines which part of the flow thread a given
column occupies. It is stored (or calculated) as a rectangle, although we really
only care about the logical top and logical bottom coordinates. Logical widths
and inline-direction coordinates are the same for every column anyway.

\[\*\] However, the flow thread needs to add pagination struts at column
boundaries (described later), to achieve a uniform column height.

## Example

&lt;div style="-webkit-columns:3; -webkit-column-rule:solid;
-webkit-column-gap:20px; line-height:20px; width:220px;"&gt;

` line1<br>`

` line2<br>`

` line3<br>`

` line4<br>`

` line5<br>`

` <div id="elm">`

` LINE6<br>`

` LINE7<br>`

` </div>`

` line8<br>`

` line9<br>`

`</div>`

The render tree would look like this:

`LayoutBlockFlow (the multicol container DIV)`

` LayoutMultiColumnFlowThread (anonymous)`

` LayoutBlock (anonymous, to hold inline content)`

` LayoutText "line1"`

` LayoutBR`

` LayoutText "line2"`

` LayoutBR`

` LayoutText "line3"`

LayoutBR

` LayoutText "line4"`

LayoutBR

` LayoutText "line5"`

LayoutBR

` LayoutBlock ("elm" DIV)`

` LayoutText "LINE6"`

LayoutBR

` LayoutText "LINE7"`

LayoutBR

` LayoutBlock (anonymous, to hold inline content)`

` LayoutText "line8"`

LayoutBR

` LayoutText "line9"`

LayoutBR

` LayoutMultiColumnSet (anonymous)`

Column width will be 60px (220px minus two gaps (2 \* 20px), then divided by the
number of columns (3)).

Column height will also be 60px (three lines, 3 \* 20px).

It will be rendered like this, visually:

`+---------------------------+`

`|line1 | line4 | LINE7 |`

`|line2 | line5 | line8 |`

`|line3 | LINE6 | line9 |`

`+---------------------------+`

The flow thread, however, is laid out like an imaginary tall single column, like
this:

`+-------+`

`|line1 |`

`|line2 |`

`|line3 |`

`|line4 |`

`|line5 |`

`|LINE6 |`

`|LINE7 |`

`|line8 |`

`|line9 |`

`+-------+`

The width of the flow thread is the same as the column width - 60px:

The height of the flow thread is 9\*20px = 180px.

The flow thread portion rectangle for the first column is (0, 0, 60px, 60px) -
logical top:0, logical bottom:60px

The flow thread portion rectangle for the second column is (0, 60px, 60px, 60px)
- logical top:60px, logical bottom:120px

The flow thread portion rectangle for the third column is (0, 120px, 60px, 60px)
- logical top:120px, logical bottom:180px

The visual rectangle for the first column is (0, 0, 60px, 60px).

The visual rectangle for the second column is (80px, 0, 60px, 60px) (x = 60px +
a gap of 20px).

The visual rectangle for the third column is (160px, 0, 60px, 60px).

The position of the "elm" DIV, in flow thread coordinates, relatively to the
flow thread, is (0, 100px). Its size (in the same coordinate space) is (60px,
40px).

To convert the top/left position (0, 100px) of this element from flow thread
coordinates to visual coordinates, we need to do this:

1. Figure out which column it's in. Int-divide its top position by the column
height - 100px / 60px = 1 (aha, the second column)

2. Add the visual left position of the column (80px).

3. Subtract the flow thread portion rectangle top coordinate (60px).

So: (0 + 80px, 100px - 60px) = (80px, 40px).

The position of LINE6 relatively to the multicol container, in flow thread
coordinates, is (0, 100px).

The position of LINE7 relatively to the multicol container, in flow thread
coordinates, is (0, 120px).

The position of LINE6 relatively to the multicol container, in visual
coordinates, is (80px, 40px).

The position of LINE7 relatively to the multicol container, in visual
coordinates, is (160px, 0).

The bounding box of "elm" (the one with LINE6 and LINE7) in flow thread
coordinates, relatively to the multicol container, will be (0, 100px, 60px,
40px). A nice rectangle. But the block isn't rectangular, visually, since "elm"
lives in two columns. There's one portion at the bottom in the second column, at
(80px, 40px, 60px, 20px) (in visual coordinates, relatively to the multicol
container), and one portion at the top in the third column, at (160px, 0, 60px,
20px) (in visual coordinates, relatively to the multicol container). The visual
bounding box is the union of these two portions - i.e. (80px, 0, 140px, 60px).

## Pagination struts

A pagination strut is inserted at a column break, when a column ends up with
unusable space at a column boundary. We end up with unusable space when
unbreakable content (e.g. a line) gets moved to the next column, because there
was **some**, but not enough, space available in the current column. For
example, if we have a bunch of lines, and the column height is 5em, and line
height is 2em, you can fit 2 lines per column. At the bottom of each column
there'll be 1em of unusable space. So we need to insert a strut in front of
lines that are "moved" to the next columns. This way the flow thread can be
sliced nicely and easily into columns when painting and hit-testing.

### Example

&lt;div style="-webkit-columns: 3; column-fill: auto; height: 3.8em; border:
solid; -webkit-column-rule: 1px solid;"&gt;

&lt;div style="background:yellow;"&gt;

&lt;span style="font-size:1.7em;"&gt;Tall line&lt;/span&gt;&lt;br&gt;

line2&lt;br&gt;

line3&lt;br&gt;

&lt;/div&gt;

line4&lt;br&gt;

line5&lt;br&gt;

line6&lt;br&gt;

line7&lt;br&gt;

&lt;/div&gt;

This gives the following flow thread, with pagination struts shown in magenta:

Visual rendering, with the flow thread super-imposed for each column. Struts
shown in magenta:

Note that we also end up with unusable space (and therefore need pagination
struts) at forced breaks.

## Column balancing

## If column height cannot be deduced from CSS properties (typically "height"), because the height is auto, or column-fill is 'balance' (that's the initial value), or if a column row is followed by a column-span:all element, we need to attempt to calculate an optimal column height on our own, based on the contents.

It goes like this:

1. Lay out the flow thread without any column breaks

2. Initial / guessed / minimal column height is: flow thread's height divided by
the number of columns.

3. Lay out the flow thread again, with implicit breaks, inserting any necessary
pagination struts at column boundaries.

4. If we get too many columns now, stretch column height by the "minimum space
shortage" and repeat the previous step, as many times as necessary.

Minimum space shortage: the smallest amount of space that would have prevented a
break at a given position in the previous layout pass.

## Nested fragmentation contexts

A fragmentation context is either established by a multicol container or by
printing. When printing, the entire document is split into pages. Printing a
multicol container is an example of a nested fragmentation context situation. If
the multicol container is split across multiple pages, special attention is
required, so that content is flowed into columns on one page before continuing
on the next page.

### Example for printing

&lt;div style="-webkit-columns:2; line-height:2em;"&gt;

line1&lt;br&gt;

line2&lt;br&gt;

line3&lt;br&gt;

line4&lt;br&gt;

line5&lt;br&gt;

line6&lt;br&gt;

line7&lt;br&gt;

line8&lt;br&gt;

line9&lt;br&gt;

line10&lt;br&gt;

&lt;/div&gt;

If the first page only has room left for 3 lines of text, we'd be able to fit
the first 6 lines (since we have 2 columns) on the first, and the remaining 4
lines on the next page.

We get this layout:

line1 line4

line2 line5

line3 line6

---------- page break ---------

line7 line9

line8 line10

And NOT this, for instance:

~~line1 line6~~

~~line2 line7~~

~~line3 line8~~

~~---------- page break ---------~~

~~line4 line9~~

~~line5 line10~~

This will give us two MultiColumnFragementainerGroup objects, one for each page.
The height of the first one will be 2em\*3 = 6em (the remaining space on that
page), while the height of second one will be 2em\*2 = 4em, which is what we get
when we balance the remaining 4 lines.

Another way of ending up in a nested fragmentation context situation, is to put
a multicol container inside another multicol container. Each multicol container
establishes their own flow thread.

### Example with nested multicol

&lt;style&gt;

.multicol { -webkit-columns:2; -webkit-column-gap:1em; line-height:2em; }

#outer { width:19em; padding:5px; background:blue; }

#inner { background:yellow; }

&lt;/style&gt;

&lt;div class="multicol" id="outer"&gt;

&lt;div class="multicol" id="inner"&gt;

line1&lt;br&gt;

line2&lt;br&gt;

line3&lt;br&gt;

line4&lt;br&gt;

line5&lt;br&gt;

line6&lt;br&gt;

line7&lt;br&gt;

line8&lt;br&gt;

line9&lt;br&gt;

line10&lt;br&gt;

&lt;/div&gt;

&lt;/div&gt;

The layout tree looks like this:

LayoutView 0x26aae9804010       #document       0x20b8ed506be0

LayoutBlockFlow 0x26aae9814010  HTML    0x20b8ed507828

LayoutBlockFlow 0x26aae9814120  BODY    0x20b8ed507a20

LayoutBlockFlow 0x26aae9814230  DIV     0x20b8ed507a88 ID="outer"
CLASS="multicol"

LayoutMultiColumnFlowThread (anonymous) 0x26aae9818010

LayoutBlockFlow 0x26aae9814340  DIV     0x20b8ed507b40 ID="inner"
CLASS="multicol"

LayoutMultiColumnFlowThread (anonymous) 0x26aae9818198

LayoutText 0x26aae9830c88       #text   0x20b8ed507ba8 "\\n line1"

LayoutBR 0x26aae9830be0         BR      0x20b8ed507bf8

LayoutText 0x26aae9830b38       #text   0x20b8ed507c60 "\\n line2"

LayoutBR 0x26aae9830a90         BR      0x20b8ed507cb0

LayoutText 0x26aae98309e8       #text   0x20b8ed507d18 "\\n line3"

LayoutBR 0x26aae9830940         BR      0x20b8ed507d68

LayoutText 0x26aae9830898       #text   0x20b8ed507dd0 "\\n line4"

LayoutBR 0x26aae98307f0         BR      0x20b8ed507e20

LayoutText 0x26aae9830748       #text   0x20b8ed507e88 "\\n line5"

LayoutBR 0x26aae98306a0         BR      0x20b8ed507ed8

LayoutText 0x26aae98305f8       #text   0x20b8ed507f40 "\\n line6"

LayoutBR 0x26aae9830550         BR      0x20b8ed507f90

LayoutText 0x26aae98304a8       #text   0x20b8ed507ff8 "\\n line7"

LayoutBR 0x26aae9830400         BR      0x20b8ed508048

LayoutText 0x26aae9830358       #text   0x20b8ed5080b0 "\\n line8"

LayoutBR 0x26aae98302b0         BR      0x20b8ed508100

LayoutText 0x26aae9830208       #text   0x20b8ed508168 "\\n line9"

LayoutBR 0x26aae9830160         BR      0x20b8ed5081b8

LayoutText 0x26aae98300b8       #text   0x20b8ed508220 "\\n line10"

LayoutBR 0x26aae9830010         BR      0x20b8ed508270

LayoutMultiColumnSet (anonymous) 0x26aae9824170

LayoutMultiColumnSet (anonymous) 0x26aae9824010

Some random facts:

The LayoutMultiColumnSet of the outer multicol container (0x26aae9824010) has
one row (fragmentainer group) with two columns.

The LayoutMultiColumnSet of the inner multicol container (0x26aae9824170) has
two rows (fragmentainer groups) with two columns each.

All 10 lines are inside the inner multicol, so they are laid out in the inner
flow thread.

The first inner fragmentainer group holds 6 lines, so it takes up 6\*2em = 12em
of space in the flow thread. Its height in the coordinate space of the outer
flow thread is 6em.

The second inner fragmentainer group holds 4 lines, so it takes up 4\*2em = 8em
in the flow thread. Its height in the outer flow thread is 4em.

The total height of the inner flow thread is 12em+8em = 20em.

The height of the inner multicol container in the coordinate space of the outer
flow thread is the combined height of its fragmentainer groups - 6em+4em = 10em.

The outer multicol container has two columns, with room for one inner
fragmentainer group in each.

Its content height (i.e. excluding the padding) needs to be 6em to fit
everything.

TODO(mstensho): clean up and add more stuff here

Here's a table of the logical left and logical top coordinates for each line, in
various coordinate spaces. All values are in "em" units.

<table>
<tr>

<td><b> Inner flow thread coordinate space</b></td>
<td><b> Outer flow thread coordinate space</b></td>
<td><b>Visual coordinate space (relative to content edge of outer multicol container)</b></td>
</tr>
<tr>
<td> <b>line1</b></td>
<td>0,0</td>
<td> 0,0</td>
<td>0,0</td>
</tr>
<tr>
<td> <b>line2</b></td>
<td>0,2</td>
<td> 0,2</td>
<td>0,2</td>
</tr>
<tr>
<td> <b>line3</b></td>
<td>0,4</td>
<td> 0,4</td>
<td>0,4</td>
</tr>
<tr>
<td> <b>line4</b></td>
<td>0,6</td>
<td> 5,0</td>
<td>5,0</td>
</tr>
<tr>
<td> <b>line5</b></td>
<td>0,8</td>
<td> 5,2</td>
<td>5,2</td>
</tr>
<tr>
<td> <b>line6</b></td>
<td>0,10</td>
<td> 5,4</td>
<td>5,4</td>
</tr>
<tr>
<td> <b>line7</b></td>
<td>0,12</td>
<td> 0,6</td>
<td>10,0</td>
</tr>
<tr>
<td> <b>line8</b></td>
<td>0,14</td>
<td> 0,8</td>
<td>10,2</td>
</tr>
<tr>
<td> <b>line9</b></td>
<td>0,16</td>
<td> 5,6</td>
<td>15,0</td>
</tr>
<tr>
<td> <b>line10</b></td>
<td>0,18</td>
<td> 5,8</td>
<td>15,2</td>
</tr>
</table>
