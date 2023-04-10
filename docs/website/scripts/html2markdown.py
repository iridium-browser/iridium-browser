# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""HTML to Markdown renderer."""

import os
import re
import io
import textwrap
import urllib
import xml.sax


class _Flags:
  # Whether to render h1s and h2s with underlined - and =.
  underline_headers = False

  # The set of characters to escape with \'\\\' in the
  # Markdown. This is not the set of all special Markdown
  # characters, but rather those characters that tend to
  # get misinterpreted as Markdown syntax the most. Blindly
  # escaping all special Markdown characters results in ugly
  # Markdown.
  escape_chars = r'\`*[]'

  # Format for italic tags.
  italic_format = '*'

  # Format for bold tags.
  bold_format = '**'

  # Format for strikethrough tags.
  strike_format = '~~'

  # Format for underline tags.
  highlight_format = '=='

  # Number of spaces to indent an unordered list.
  # This total includes the bullet.
  # For example, a value of 4 yields '*   '
  unordered_list_indent = 4

  # Number of spaces to indent an ordered list.
  # This total includes the number.
  # For example, a value of 4 yields '1.  '
  ordered_list_indent = 4

  # The DIV blocks that should be formatted as code.
  code_class_regex = r'^sites-codeblock sites-codesnippet-block$'

  # The class of DIV blocks used for table of contents.
  toc_class_regex = r'^sites-embed-content sites-embed-type-toc$'

  # The class of DIV blocks that should be ignored.
  ignore_class_regex = r''

  # The style of DIV blocks that should be ignored.
  ignore_style_regex = r'^display:none;$'

  # Format text blocks to the given line width. Set to zero
  # to disable line wrapping.
  line_width = 80

  # Whether to use indented code blocks, if False use fenced.
  indented_code_blocks = False

  # Whether to use HTML code blocks instead of fenced code
  # blocks if source code block includes formatted text.
  allow_html_code_blocks = True

  # Links that are automatically recognized by the renderer.
  shortlinks_regex = r'^http://(ag|b|cl|g|go|who)/'

  # Print the fragment tree for debugging.
  debug_print_tree = False


FLAGS = _Flags()


def _EscapeText(text, reserved_chars):
  """Escapes any reserved characters with a backslash.

  Args:
    text: The string to escape.
    reserved_chars: A string of reserved characters that need to be escaped.

  Returns:
    The escaped text.
  """
  markdown = io.StringIO()
  for c in text:
    if c in reserved_chars:
      markdown.write('\\')
    markdown.write(c)
  return markdown.getvalue()


def _EscapeContentForHtml(text):
  result = io.StringIO()
  escapes = {'<': '&lt;', '>': '&gt;'}
  for c in text:
    result.write(c if c not in escapes else escapes[c])
  return result


ENCODED_NEWLINE = '&#%d;' % ord('\n')


def _RestoreEncodedNewlines(text):
  return text.replace(ENCODED_NEWLINE, '\n')


def _WrapLine(line, indent):
  """Wraps the line to fit into the column limit.

  Args:
    line: The string to wrap.
    indent: An integer with the number of columns of indentation.

  Returns:
    The wrapped text.
  """
  if FLAGS.line_width > 0:
    return ('\n' + ' ' * indent).join(textwrap.wrap(
        line,
        width=FLAGS.line_width - indent,
        break_long_words=False,
        break_on_hyphens=False))
  return line


class Fragment:
  """Base class for all output fragments.

  To generate a line of output, the methods will be called in the following
  order:

  WriteIndent()
  WriteContentIntoParentAndClear()
  ConsumeContent() -- for the topmost fragment only
  StripLine()
  WrapLine()
  """

  def __init__(self, indent, prefix, suffix):
    self._content = io.StringIO()
    self._indent = indent
    self._prefix = prefix
    self._suffix = suffix
    self._parent = None
    self._children = []

  def __repr__(self):
    debug_print = lambda text: text.encode('utf-8') if text else ''
    return ('{' +
            self.__class__.__name__ +
            ': indent=' + debug_print(self._indent) +
            '; prefix=' + debug_print(self._prefix) +
            '; content=' + debug_print(self._content.getvalue()) +
            '; suffix=' + debug_print(self._suffix) +
            '}')

  def SetParent(self, parent):
    self._parent = parent

  def AddChild(self, node):
    self._children.append(node)
    node.SetParent(self)
    return node

  def GetChildren(self):
    return self._children

  def _AllChildren(self):
    all_children = []
    def Traverse(fragment):
      for c in fragment.GetChildren():
        all_children.append(c)
        Traverse(c)
    Traverse(self)
    return all_children

  def WriteIndent(self, output):
    if self._indent:
      output.write(self._indent)

  def WriteContentIntoParentAndClear(self):
    self._WriteContent(self._parent._content)  # pylint: disable=protected-access
    self._ClearContent()
    self._children = []

  def _WriteContent(self, output):
    """Implementation of content rendering. Can be overridden in subclasses."""
    self._Write(output, self._prefix, self._content.getvalue(), self._suffix)

  def _Write(self, output, prefix, content, suffix):
    """Default implementation of content rendering for reuse by subclasses."""
    has_content = bool(content.strip())
    if prefix and has_content:
      output.write(prefix)
    output.write(content)
    if suffix and has_content:
      output.write(suffix)

  def UnsetSuffix(self):
    self._suffix = ''

  def UnsetPrefix(self):
    self._prefix = ''

  def _UpdatePrefixAndSuffix(self, prefix, suffix):
    if self._prefix:
      self._prefix = prefix
    if self._suffix:
      self._suffix = suffix

  def _ClearContent(self):
    """Clears the content. This will only be called after it's been written."""
    self._content = io.StringIO()

  def ConsumeContent(self):
    content = self._content
    self._ClearContent()
    return content

  def Append(self, text):
    """Appends text.

    Args:
      text: The string to append, it will be escaped.
    """
    assert isinstance(text, str)
    self._content.write(self.EscapeText(text))

  def EscapeText(self, text):
    """Escapes any reserved characters when Append() is called with text.

    By default this defers to the parent fragment.

    Args:
      text: The string to escape.

    Returns:
      The escaped string.
    """
    if self._parent:
      return self._parent.EscapeText(text)
    return text

  def StripLine(self, text):
    """Does any needed stripping of whitespace.

    Some blocks (code for example) will want to preserve whitespace, while
    others will want to coalesce it together. By default this defers to the
    parent fragment.

    Args:
      text: The string to strip

    Returns:
      The stripped string.
    """
    if self._parent:
      return self._parent.StripLine(text)
    return text

  def WrapLine(self, line, indent):
    """Wraps the line to fit into the column limit, if necessary.

    Most blocks (code for example) will want to preserve whitespace and won't
    break their output.

    Args:
      text: The string to wrap.
      indent: Indent string.
    Returns:
      The wrapped string.
    """
    del indent
    return line

  def NeedsToMergeWith(self, text):
    del text
    return False


class HTML(Fragment):
  """Markdown fragment that consists of just an unescaped HTML string."""

  def __init__(self, prefix=None, suffix=None):
    super().__init__(indent=None, prefix=prefix, suffix=suffix)

  def EscapeText(self, text):
    return text


class Href(Fragment):
  """HTML fragment containing an <a href=> tag. Used within table cells.

  If the href falls within a table cell, using a Href() element will allow
  us to have proper formatting; the Markdown-style Link() element will not
  be processed properly.
  """
  def __init__(self, href):
    super().__init__(indent=None, prefix='<a href="%s">' % href, suffix='</a>')


class Text(Fragment):
  """Markdown fragment that consists of just a string."""

  def __init__(self, indent=None, prefix=None, suffix=None):
    super().__init__(indent, prefix, suffix)


class IgnoreBlock(Fragment):
  """Markdown fragment that omits all content."""

  def __init__(self):
    super().__init__(None, None, None)


class TextBlock(Text):
  """A TextBlock coalesces all spaces and escapes all reserved chars."""

  def EscapeText(self, text):
    text = _EscapeContentForHtml(text).getvalue()
    return _EscapeText(text, FLAGS.escape_chars)

  def StripLine(self, text):
    # Treat newlines as spaces and then coalesce spaces.
    text = text.replace('\n', ' ')
    # Replace all Unicode nonbreaking spaces with simple spaces. This is safer
    # than deletion since spaces are coalesced below anyway.
    text = text.replace(chr(160), ' ')

    return re.sub(r' +', ' ', text.strip())


class Div(TextBlock):
  """Placeholder that helps with the two-column layout conversion."""

  def __init__(self, cls):
    self.cls = cls
    super().__init__()


class Table(TextBlock):
  """Placeholder that identifies when we're in a (data) table.

  (As opposed to a table being used for layout-purposes, which we don't
  want to export.)
  """
  cls = None


class TD(Text):
  def __init__(self, rowspan, colspan):
    prefix = '<td'
    if rowspan and str(rowspan) != '1':
        prefix += ' rowspan=%s' % rowspan
    if colspan and str(colspan) != '1':
        prefix += ' colspan=%s' % colspan
    prefix += '>'
    super().__init__(indent='', prefix=prefix, suffix='</td>')


class Content(TextBlock):
  """Placeholder that identifies when we're processing the main content."""
  cls = None


class WrappedTextBlock(TextBlock):
  """A WrappedTextBlock wraps the output lines to fit into the column limit."""

  def WrapLine(self, line, indent):
    return _WrapLine(line, len(indent))


class BlockquoteBlock(WrappedTextBlock):
  """A BlockquoteBlock wraps content and prepends each line with '> '.

  The generator must emit BlockquoteBlocks with no indent for paragraphs
  inside a blockquote. This will allow propagating the final call to WrapLine
  up to the outermost BlockquoteBlock which will wrap the lines and prepend
  each of them with the indent.
  """

  def __init__(self, indent='> '):
    super().__init__(indent, None, None)

  def WrapLine(self, line, indent):
    if not self._indent and self._parent:
      return self._parent.WrapLine(line, indent)
    wrapped = _WrapLine(line, len(indent))
    lines = wrapped.splitlines(True)
    return indent.join([l.lstrip() for l in lines])


class CodeBlock(Text):
  """Base class for different code block fragment implementations."""

  def EscapeText(self, text):
    return text

  def StripLine(self, text):
    # Completely ignore newlines in code blocks. Sites always uses <br/>.
    return text.replace('\n', '')

  def ChangeToHtml(self):
    content = self._content.getvalue()
    if content:
      self._content = _EscapeContentForHtml(content)


class IndentedCodeBlock(CodeBlock):
  """A IndentedCodeBlock indents by four spaces."""

  def __init__(self, indent='    '):
    super().__init__(indent, None, None)


class FencedCodeBlock(CodeBlock):
  """A FencedCodeBlock is fenced with triple backticks (```).

  To render correctly, content writing must not happen
  unless the end of the source code block has been encountered.
  That is, the entire code block from the source HTML must
  be rendered in a single write pass.
  """

  def __init__(self, indent=None,
               prefix='```none' + ENCODED_NEWLINE,
               suffix=ENCODED_NEWLINE + '```'):
    super().__init__(indent, prefix, suffix)

  def WriteIndent(self, output):
    # Adjust inner fragments and self before rendering.
    if FLAGS.allow_html_code_blocks:
      has_formatted_text = False
      for c in self._AllChildren():
        if isinstance(c, FormattedText):
          c.ChangeToHtml()
          has_formatted_text = True
      if has_formatted_text:
        for c in self._AllChildren():
          if isinstance(c, CodeBlock):
            c.ChangeToHtml()
        self._UpdatePrefixAndSuffix(
            '<pre><code>', ENCODED_NEWLINE + '</code></pre>')
    super().WriteIndent(output)

  def StripLine(self, text):
    text = super().StripLine(text)
    lines = _RestoreEncodedNewlines(text).splitlines()
    return '\n'.join([l for l in lines if l])

  def WrapLine(self, line, indent):
    lines = line.splitlines(True)
    return indent.join(lines)


class FencedCodeBlockLine(Text):
  """A line of code inside FencedCodeBlock."""

  def __init__(self, indent=None,
               prefix=ENCODED_NEWLINE, suffix=ENCODED_NEWLINE):
    super().__init__(indent, prefix, suffix)

  def StripLine(self, text):
    text = super().StripLine(text)
    return _RestoreEncodedNewlines(text)


class UnderlinedHeader(TextBlock):
  """Markdown fragment for an underlined section header."""

  def __init__(self, char):
    super().__init__()
    self._char = char

  def _WriteContent(self, output):
    length = len(self.StripLine(self._content.getvalue()))
    if length > 0:
      # '\n' will be stripped, so use an encoded '\n' that we can later replace
      # after the line is stripped.
      self._Write(output,
                  None,
                  self._content.getvalue(),
                  ENCODED_NEWLINE + self._char * length)

  def StripLine(self, text):
    text = super().StripLine(text)
    return _RestoreEncodedNewlines(text)


class FormattedText(Text):
  """Text wrapped in Markdown formatting."""

  def __init__(self, fmt):
    super().__init__(None, fmt, fmt)

  def _Pad(self, bigger, smaller):
    return ' ' * (len(bigger) - len(smaller))

  def _WriteContent(self, output):
    prefix = self._prefix
    content = self._content.getvalue()
    suffix = self._suffix
    if prefix:
      # If there are whitespaces immediately after the prefix,
      # they must be pushed out before the prefix.
      lstripped = content.lstrip()
      if len(content) > len(lstripped):
        prefix = self._Pad(content, lstripped) + prefix
        content = lstripped
    if suffix:
      # If there are whitespaces immediately before the suffix,
      # they must be pushed out after the suffix.
      rstripped = content.rstrip()
      if len(content) > len(rstripped):
        suffix = suffix + self._Pad(content, rstripped)
        content = rstripped
    self._Write(output, prefix, content, suffix)

  def ChangeToHtml(self):
    content = self._content.getvalue()
    if content:
      content = _EscapeContentForHtml(content)


class BoldFormattedText(FormattedText):
  """Text formatted as bold."""

  def __init__(self):
    super().__init__(FLAGS.bold_format)

  def NeedsToMergeWith(self, text):
    return isinstance(text, BoldFormattedText)

  def ChangeToHtml(self):
    super().ChangeToHtml()
    self._UpdatePrefixAndSuffix('<b>', '</b>')


class ItalicFormattedText(FormattedText):
  """Text formatted as italic."""

  def __init__(self):
    super().__init__(FLAGS.italic_format)

  def NeedsToMergeWith(self, text):
    return isinstance(text, ItalicFormattedText)

  def ChangeToHtml(self):
    super().ChangeToHtml()
    self._UpdatePrefixAndSuffix('<i>', '</i>')


class StrikeThroughFormattedText(FormattedText):
  """Text formatted as strike through."""

  def __init__(self):
    super().__init__(FLAGS.strike_format)

  def NeedsToMergeWith(self, text):
    return isinstance(text, StrikeThroughFormattedText)

  def ChangeToHtml(self):
    super().ChangeToHtml()
    self._UpdatePrefixAndSuffix('<s>', '</s>')


class HighlightFormattedText(FormattedText):
  """Highlighted text."""

  def __init__(self):
    super().__init__(FLAGS.highlight_format)

  def NeedsToMergeWith(self, text):
    return isinstance(text, HighlightFormattedText)

  def ChangeToHtml(self):
    super().ChangeToHtml()
    self._UpdatePrefixAndSuffix('<u>', '</u>')


class ListItem(Text):
  """Item in a list."""

  def __init__(self, bullet):
    super().__init__()
    self._bullet = bullet

  def WriteIndent(self, output):
    if self._bullet:
      # TODO(dpranke): The original code relied on strings and bytes
      # being interchangeable in Python2, so you could seek backwards
      # from the current location with a relative offset. You can't
      # do that in Python3, apparently.
      #
      # To get around this for the moment, instead of seeking backwards
      # 4 characters, we embed 4 '\b' backspaces, and then have the client
      # do a global search and replace of '    \b\b\b\b' with '' instead.
      #
      # This is awkward, so we should rework this so that this isn't needed.
      #
      # output.seek(-len(self._bullet), os.SEEK_CUR)
      output.write('\b' * len(self._bullet))
      output.write(self._bullet)
    super().WriteIndent(output)

  def _ClearContent(self):
    self._bullet = None
    super()._ClearContent()

  def WrapLine(self, line, indent):
    return _WrapLine(line, len(indent))


class Link(Text):
  """Markdown link."""

  def __init__(self, href):
    super().__init__()
    self._href = href
    self._url_opener_prefix = ''
    self._url_opener_suffix = ''

  def MakeAnImage(self, width, height):
    self._url_opener_prefix = '!'
    if width and height:
      self._url_opener_suffix = (
          '{{width="{}" height="{}"}}'.format(width, height))

  def _IsShortLink(self, text):
    if FLAGS.shortlinks_regex and (
        re.compile(FLAGS.shortlinks_regex).match(self._href)):
      parsed_href = urllib.parse.urlsplit(self._href)
      if parsed_href.netloc + parsed_href.path == text:
        return True
    return None

  def _WriteLink(self, output, text):
    write_short_link = (not (self._url_opener_prefix or self._url_opener_suffix)
                        and self._IsShortLink(text))
    if write_short_link:
      self._Write(output, None, text, None)
    else:
      self._Write(output,
                  self._url_opener_prefix + '[',
                  text,
                  '](' + self._href + ')' + self._url_opener_suffix)

  def _WriteContent(self, output):
    text = self._content.getvalue()
    if text:
      if text.startswith('http://') or text.startswith('https://'):
        self._Write(output, '<', text, '>')
      else:
        self._WriteLink(output, text)


class Image(Text):
  """Image."""

  def __init__(self, src, alt, width, height):
    super().__init__()
    self._src = src
    self._alt = alt or 'image'
    self._width = width
    self._height = height

  def _WriteContent(self, output):
    tag = '<img alt="%s" src="%s"' % (self._alt, self._src)
    if self._height:
        tag += ' height=%s' % self._height
    if self._width:
        tag += ' width=%s' % self._width
    tag += '>'
    self._Write(output, '', tag, '')


class Code(Text):
  """Inline code."""

  def __init__(self):
    super().__init__(None, '`', '`')

  def EscapeText(self, text):
    return text

  def _WriteContent(self, output):
    prefix = self._prefix
    content = self._content.getvalue()
    suffix = self._suffix
    if '`' in content:
      # If a backtick (`) is present inside inline code, the fragment
      # must use double backticks.
      prefix = suffix = '``'
      # Since having content starting or ending with a backtick would emit
      # triple backticks which designates a fenced code fragment, pad content
      # to avoid this.
      if content.startswith('`'):
        content = ' ' + content
      if content.endswith('`'):
        content += ' '
    self._Write(output, prefix, content, suffix)

  def NeedsToMergeWith(self, text):
    return isinstance(text, Code)


class EmbeddedContent(Text):
  """Embedded content: Docs, Drawings, Presentations, etc."""

  def __init__(self, href, width, height):
    super().__init__()
    self._href = href
    self._width = width
    self._height = height

  def _WriteContent(self, output):
    parsed_href = urllib.parse.urlsplit(self._href)
    if parsed_href.scheme == 'http':
      parsed_href = urllib.parse.SplitResult(
          'https', parsed_href.netloc, parsed_href.path, parsed_href.query,
          parsed_href.fragment)
    # Note: 'allow="fullscreen"' is requested for all content for simplicity.
    # g3doc server has dedicated logic to deal with these requests.
    element = '<iframe src="{}"{} allow="fullscreen" />'.format(
        urllib.parse.urlunsplit(parsed_href),
        (' width="{}" height="{}"'.format(self._width, self._height) if (
            self._width and self._height) else ''))
    self._Write(output, None, element, None)


class ListInfo:

  def __init__(self, tag):
    self.tag = tag       # The tag used to start the list
    self.item_count = 0  # The number of items in the list


class FragmentTree:
  """Class for managing a tree of fragments.

  There is a "scope" formed by nested fragments, e.g.
  italic fragment inside bold fragment inside paragraph.
  The scope is stored in the stack. For convenience,
  the stack always have one element.

  Fragments popped out from the scope may be re-added
  back into the tree as children of the last fragment.
  This allows "chaining" of structured content for future
  processing. For example, if there were several bold
  fragments inside a paragraph interleaved with fragments
  of regular text, all these fragments will end up as
  children of the paragraph fragment.

  """

  def __init__(self, top_fragment):
    self._stack = [top_fragment]

  def ActiveFragmentScopeDepth(self):
    return len(self._stack) - 1

  def StartFragment(self, fragment):
    fragment.SetParent(self._stack[-1])
    self._stack.append(fragment)
    return fragment

  def EndFragment(self):
    return self._stack.pop()

  def AppendFragment(self, fragment):
    return self._stack[-1].AddChild(fragment)

  def _ApplyRecursivelyToNode(self, node, scope_operation, operation,  # pylint: disable=missing-docstring
                              debug_indent):
    if not debug_indent:
      for child in node.GetChildren():
        self._ApplyRecursivelyToNode(child, scope_operation, operation, None)
    else:
      debug_indent += '  c '
      for child in node.GetChildren():
        print(debug_indent + repr(child))
        self._ApplyRecursivelyToNode(child, scope_operation, operation,
                                     debug_indent)
    operation(node)

  def _ApplyRecursivelyToScope(self, nodes, scope_operation, operation,  # pylint: disable=missing-docstring
                               debug_indent):
    node = nodes.pop()
    scope_operation(node)
    if debug_indent:
      print(debug_indent + repr(node))
    if nodes:
      self._ApplyRecursivelyToScope(nodes, scope_operation, operation,
                                    (debug_indent + '  s ' if debug_indent
                                     else None))
    self._ApplyRecursivelyToNode(node, scope_operation, operation,
                                 debug_indent)

  def ApplyToAllFragments(self, scope_operation, operation):
    """Recursively applies operations to all fragments in the tree.

    The omnipresent topmost fragment is excluded. The 'scope_operation'
    is applied to every element in the fragment stack in pre-order.
    The 'operation' is applied to all fragments in the tree in post-order.

    Args:
      scope_operation: The operation to apply to fragments in the scope stack.
      operation: The operation to apply to all fragments in the tree.
    """
    self._ApplyRecursivelyToScope(list(reversed(self._stack[1:])),
                                  scope_operation, operation,
                                  '  ' if FLAGS.debug_print_tree else None)

  def FindFirstFragmentFromEnd(self, predicate, steps_from_last=0):
    sub_stack = self._stack[:-steps_from_last if steps_from_last else None]
    return next((node for node in sub_stack if predicate(node)), None)

  def PeekFragmentFromStart(self, steps_from_first=0):
    return self._stack[steps_from_first]

  def PeekFragmentFromEnd(self, steps_from_last=0):
    return self._stack[-(steps_from_last + 1)]

  def PeekLastAppendedFragment(self):
    return (self._stack[-1].GetChildren()[-1]
            if self._stack[-1].GetChildren() else None)


class MarkdownGenerator:
  """Generates Markdown based on the series of HTML tags seen.

  Each time an opening HTML tag is seen, the appropriate markdown fragment is
  created and pushed onto a stack. Any text encountered is appended to the
  fragment at the top of the stack. When a closing HTML tag is seen, the stack
  is popped and the fragment removed is appended to the new top of the stack.

  Markdown is buffered in the fragment stack until an entire line has been
  formed, at which point _WriteFragmentsAsLine() is called to write it out. The
  content buffered in the stack is cleared, but otherwise the stack remains
  unmodified.
  """

  def __init__(self, out, url_translator):
    self._out = out
    self._url_translator = url_translator
    self._fragment_tree = FragmentTree(Text())
    self._list_info_stack = []
    self._pending_newlines = 0
    # Initialize the regexps to match nothing (rather than be None).
    self._code_class_regex = re.compile(FLAGS.code_class_regex or 'a^')
    self._toc_class_regex = re.compile(FLAGS.toc_class_regex or 'a^')
    self._ignore_class_regex = re.compile(FLAGS.ignore_class_regex or 'a^')
    self._ignore_style_regex = re.compile(FLAGS.ignore_style_regex or 'a^')

  def _Push(self, fragment):
    """Sets the parent fragment and pushes it onto the fragment stack.

    In the case where there is an IgnoreBlock on the stack, a new IgnoreBlock
    is pushed instead.

    Args:
      fragment: The Fragment object to push on the stack.
    """
    if isinstance(self._fragment_tree.PeekFragmentFromEnd(), IgnoreBlock):
      # If the top of the stack is IgnoreBlock, push an IgnoreBlock instead.
      fragment = IgnoreBlock()
    else:
      # Check if we need to merge adjacent formatting, e.g.
      # instead of **bold****bold** we need to write **boldbold**,
      # as the former is not correct Markdown syntax.
      last_appended = self._fragment_tree.PeekLastAppendedFragment()
      if last_appended and last_appended.NeedsToMergeWith(fragment):
        last_appended.UnsetSuffix()
        fragment.UnsetPrefix()

    self._fragment_tree.StartFragment(fragment)

  def _Pop(self):
    """Pops the fragment stack it to the new top of stack.

    If the fragment stack would be empty after popping, then the fragment is
    written to the output first.
    """
    if self._fragment_tree.ActiveFragmentScopeDepth() > 1:
      fragment = self._fragment_tree.EndFragment()
      self._fragment_tree.AppendFragment(fragment)
    else:
      self._WriteFragmentsAsLine(newlines=0)
      self._fragment_tree.EndFragment()

  def _IsWithinFragmentType(self, fragment_type, steps_from_last=0):
    return self._fragment_tree.FindFirstFragmentFromEnd(
        lambda fragment: isinstance(fragment, fragment_type),
        steps_from_last) is not None

  def _LastFragmentIs(self, fragment_type, cls):
    fragment = self._fragment_tree.PeekFragmentFromEnd()
    return (isinstance(fragment, fragment_type) and fragment.cls == cls)

  def Break(self):
    if not self._IsWithinFragmentType(FencedCodeBlock):
      self._WriteFragmentsAsLine(newlines=1)
    else:
      fragment = FencedCodeBlockLine(prefix='', suffix='')
      self._Push(fragment)
      fragment.Append(ENCODED_NEWLINE)
      self._Pop()

  def HorizontalRule(self):
    # Horizontal rule must be preceded and followed by a blank line
    self._AddVerticallyPaddedParagraph('---')

  def StartDocument(self):
    self._Push(WrappedTextBlock())

  def EndDocument(self):
    self._Pop()

  def StartParagraph(self):
    self._WriteFragmentsAsLine(newlines=2)

  def EndParagraph(self):
    self._WriteFragmentsAsLine(newlines=2)

  def StartDiv(self, cls, style, ident):
    """Process opening of a div element.

    Args:
      cls: The class attribute of the element.
      style: The style attribute of the element.
      ident: The id attribute of the element
    """
    if not self._IsWithinFragmentType(FencedCodeBlock):
      if self._IsWithinFragmentType(CodeBlock):
        self._WriteFragmentsAsLine(newlines=1)
      else:
        self._WriteFragmentsAsLine(newlines=2)

    if ((cls and self._ignore_class_regex.match(cls)) or
        style and self._ignore_style_regex.match(style)):
      self._Push(IgnoreBlock())
    elif self._IsWithinFragmentType(FencedCodeBlock):
      self._Push(FencedCodeBlockLine())
    elif self._IsWithinFragmentType(CodeBlock):
      self._Push(CodeBlock())
    elif self._IsWithinFragmentType(BlockquoteBlock):
      self._Push(BlockquoteBlock(indent=None))
    elif cls and self._toc_class_regex.match(cls):
      self._AddTableOfContents()
      self._Push(IgnoreBlock())  # Ignore the items inside the Sites TOC
    elif cls and self._code_class_regex.match(cls):
      if FLAGS.indented_code_blocks:
        self._Push(IndentedCodeBlock())
      else:
        self._Push(FencedCodeBlock())
    else:
      self._Push(WrappedTextBlock())

  def EndDiv(self):
    if not self._IsWithinFragmentType(FencedCodeBlock, steps_from_last=1):
      if self._IsWithinFragmentType(CodeBlock, steps_from_last=1):
        self._WriteFragmentsAsLine(newlines=1)
      else:
        self._WriteFragmentsAsLine(newlines=2)
    self._Pop()

  def StartHeader(self, level):
    self._WriteFragmentsAsLine(newlines=2)
    if level == 1 and FLAGS.underline_headers:
      self._Push(UnderlinedHeader('='))
    elif level == 2 and FLAGS.underline_headers:
      self._Push(UnderlinedHeader('-'))
    else:
      self._Push(TextBlock(prefix=('#' * level) + ' '))

  def EndHeader(self):
    self._WriteFragmentsAsLine(newlines=2)
    self._Pop()

  def StartList(self, tag):
    if not self._list_info_stack:
      self._WriteFragmentsAsLine(newlines=2)
    else:
      self._WriteFragmentsAsLine(newlines=1)
    self._list_info_stack.append(ListInfo(tag))
    if tag == 'ol':
      self._Push(Text(' ' * FLAGS.ordered_list_indent))
    else:
      self._Push(Text(' ' * FLAGS.unordered_list_indent))

  def EndList(self):
    self._list_info_stack.pop()
    if not self._list_info_stack:
      self._WriteFragmentsAsLine(newlines=2)
    else:
      self._WriteFragmentsAsLine(newlines=1)
    self._Pop()

  def StartListItem(self):
    self._WriteFragmentsAsLine(newlines=1)
    # Google Sites sometimes spits out pages with <li> tags not enclosed within
    # an <ol> or <ul> tag.
    tag = ''
    if self._list_info_stack:
      self._list_info_stack[-1].item_count += 1
      tag = self._list_info_stack[-1].tag
    if tag == 'ol':
      item_count = self._list_info_stack[-1].item_count
      # string.ljust makes room for as many digits as you need.
      prefix = ('%d.' % item_count).ljust(FLAGS.ordered_list_indent)
      self._Push(ListItem(prefix))
    else:
      prefix = '*'.ljust(FLAGS.unordered_list_indent)
      self._Push(ListItem(prefix))

  def EndListItem(self):
    self._WriteFragmentsAsLine(newlines=1)
    self._Pop()

  def StartFormat(self, tag):
    # Allowed formatting depends on the surrounding fragment type.
    if self._IsWithinFragmentType(TD) and tag == 'b':
      # TODO(dpranke): This is a hack because I don't yet really understand
      # how the ChangeToHtml() logic works in CodeBlocks, but it seems like
      # we should be able to do something similar to what they do.
      # Also, this should really be rewriting these to <th>s instead.
      self._Push(HTML('<b>', '</b>'))
      return

    if not self._IsWithinFragmentType(IndentedCodeBlock):
      formats_map = {
          'i': ItalicFormattedText,
          'em': ItalicFormattedText,
          'b': BoldFormattedText,
          'strong': BoldFormattedText,
          'strike': StrikeThroughFormattedText,
          's': StrikeThroughFormattedText,
          'del': StrikeThroughFormattedText,
          'u': HighlightFormattedText,
          'code': Code,
          None: Text,
      }
      if self._IsWithinFragmentType(FencedCodeBlock):
        if FLAGS.allow_html_code_blocks:
          # HTML code block can render formats but must not use Code fragments.
          formats_map['code'] = formats_map[None] = CodeBlock
        else:
          formats_map = {None: CodeBlock}
    else:
      # Inside an indented code block no formatting is allowed.
      formats_map = {None: CodeBlock}
    self._Push(formats_map[tag]() if tag in formats_map
               else formats_map[None]())

  def EndFormat(self):
    self._Pop()

  def StartAnchor(self, href):
    if href is not None:
      href = self._url_translator.Translate(href)
      if self._IsWithinFragmentType(TD):
        self._Push(Href(href))
      else:
        self._Push(Link(href))
    else:
      self._Push(Text())

  def EndAnchor(self):
    self._Pop()

  def StartBlockquote(self):
    if not self._IsWithinFragmentType(CodeBlock):
      self._WriteFragmentsAsLine(newlines=1)
      self._Push(BlockquoteBlock())
    else:
      self._Push(Text())

  def EndBlockquote(self):
    if not self._IsWithinFragmentType(CodeBlock):
      self._WriteFragmentsAsLine(newlines=2)
    self._Pop()

  def Image(self, src, alt, width, height):
    src = self._url_translator.Translate(src)
    self._fragment_tree.AppendFragment(Image(src, alt, width, height))

  def Iframe(self, src, width, height):
    """Process an <iframe> element.

    Sites use <iframe> for embedded content: Docs, Drawings, etc.
    g3doc implements this by supporting <iframe> HTML tag directly.

    Args:
      src: Source URL.
      width: Element width.
      height: Element height.
    """
    if False:
      # TODO(dpranke): Figure out if we should support embedded IFRAME tags.
      # For now, we skip over them.
      self._WriteFragmentsAsLine(newlines=2)
      self._Push(EmbeddedContent(src, width, height))
      self._Pop()

  def StartTable(self, cls):
    if (cls and 'sites-layout-hbox' in cls and
        'sites-layout-name-one-column' not in cls):
      self._AddHTMLBlock('<div class="two-column-container">')
      self._Push(Div(cls='two-column-container'))
    elif (cls and 'sites-layout-name-one-column' in cls):
      pass
    else:
      self._AddHTMLBlock('<table>')
      self._Push(Table())

  def EndTable(self):
    if self._LastFragmentIs(Div, cls='two-column-container'):
      self._AddHTMLBlock('</div>')
      self._Pop()
    elif self._IsWithinFragmentType(Table):
      self._AddHTMLBlock('</table>')
      self._Pop()

  def StartTR(self):
    if self._IsWithinFragmentType(Table):
      self._AddHTMLBlock('<tr>')

  def EndTR(self):
    if self._IsWithinFragmentType(Table):
      self._AddHTMLBlock('</tr>')

  def StartTD(self, cls, rowspan, colspan):
    if self._LastFragmentIs(Div, cls='two-column-container'):
      if cls and ('sites-tile-name-content-1' in cls or
                  'sites-tile-name-content-2' in cls):
        self._AddHTMLBlock('<div class="column">')
        self._Push(Div(cls='column'))
      else:
        self._Push(Text())
    elif self._IsWithinFragmentType(Table):
      self._Push(TD(rowspan, colspan))

  def EndTD(self):
    if self._LastFragmentIs(Div, cls='column'):
      self._AddHTMLBlock('</div>')
      self._Pop()
    elif self._IsWithinFragmentType(Table):
      self._Pop()
      self._WriteFragmentsAsLine(newlines=1)

  def Text(self, text):
    if not isinstance(self._fragment_tree.PeekFragmentFromEnd(), IgnoreBlock):
      fragment = (CodeBlock() if self._IsWithinFragmentType(CodeBlock)
                  else Text())
      self._fragment_tree.AppendFragment(fragment)
      fragment.Append(text)

  def _AddTableOfContents(self):
    # TOC must be preceded and followed by a blank line
    self._AddVerticallyPaddedParagraph('[TOC]')

  def _AddVerticallyPaddedParagraph(self, text):
    self._WriteFragmentsAsLine(newlines=2)
    fragment = CodeBlock()  # Use CodeBlock to prevent escaping
    self._fragment_tree.AppendFragment(fragment)
    fragment.Append(text)
    self._WriteFragmentsAsLine(newlines=2)

  def _AddHTMLBlock(self, html):
    """Writes out a block-level string of html."""
    fragment = HTML()
    fragment.Append(html)
    self._fragment_tree.AppendFragment(fragment)
    self._WriteFragmentsAsLine(newlines=1)

  def _WriteFragmentsAsLine(self, newlines):
    """Writes out any content currently buffered in the fragment stack.

    Args:
      newlines: The minimum number of newlines required in the output after this
          line. These newlines won't be written out until the next line with
          content is encountered.
    """

    # Generate indent and the content, then clear content in fragments.
    indent = io.StringIO()
    self._fragment_tree.ApplyToAllFragments(
        lambda fragment: fragment.WriteIndent(indent),
        lambda fragment: fragment.WriteContentIntoParentAndClear())
    last_fragment = self._fragment_tree.PeekFragmentFromEnd()
    content = self._fragment_tree.PeekFragmentFromStart().ConsumeContent()
    content = last_fragment.StripLine(content.getvalue())
    indent = indent.getvalue()
    content = last_fragment.WrapLine(content, indent)

    # Write the content, if any.
    if content:
      self._out.write('\n' * self._pending_newlines)
      self._out.write(indent)
      self._out.write(content)
      self._pending_newlines = newlines
    elif self._pending_newlines > 0 and self._pending_newlines < newlines:
      self._pending_newlines = newlines

    if FLAGS.debug_print_tree:
      # Separate trees printed during each writing session
      print('-' * 20)


class XhtmlHandler(xml.sax.ContentHandler):
  """Translates SAX events into MarkdownGenerator calls."""

  # regex that matches an HTML header tag and extracts the level.
  _HEADER_TAG_RE = re.compile(r'h([1-6])$')

  def __init__(self, out, url_translator):
    xml.sax.ContentHandler.__init__(self)
    self._generator = MarkdownGenerator(out, url_translator)

  def startDocument(self):
    self._generator.StartDocument()

  def endDocument(self):
    self._generator.EndDocument()

  def startElementNS(self, name, qname, attrs):
    tag = name[1]
    if tag == 'a':
      href = attrs.get((None, 'href'))
      self._generator.StartAnchor(href)
    elif tag == 'br':
      self._generator.Break()
    elif tag == 'hr':
      self._generator.HorizontalRule()
    elif tag == 'li':
      self._generator.StartListItem()
    elif tag == 'div':
      cls = attrs.get((None, 'class'))
      style = attrs.get((None, 'style'))
      ident = attrs.get((None, 'id'))
      self._generator.StartDiv(cls, style, ident)
    elif tag == 'p':
      self._generator.StartParagraph()
    elif tag in ('b', 'code', 'em', 'i', 'strong', 's', 'strike', 'del', 'u'):
      self._generator.StartFormat(tag)
    elif tag in ('ul', 'ol'):
      self._generator.StartList(tag)
    elif tag == 'img':
      src = attrs.get((None, 'src'))
      alt = attrs.get((None, 'alt'))
      width = attrs.get((None, 'width'))
      height = attrs.get((None, 'height'))
      self._generator.Image(src, alt, width, height)
    elif tag == 'blockquote':
      self._generator.StartBlockquote()
    elif tag == 'iframe':
      src = attrs.get((None, 'src'))
      width = attrs.get((None, 'width'))
      height = attrs.get((None, 'height'))
      self._generator.Iframe(src, width, height)
    elif tag == 'table':
      cls = attrs.get((None, 'class'))
      self._generator.StartTable(cls)
    elif tag == 'tr':
      self._generator.StartTR()
    elif tag == 'td':
      self._generator.StartTD(attrs.get((None, 'class')),
                              attrs.get((None, 'rowspan')),
                              attrs.get((None, 'colspan')))
    else:
      match = self._HEADER_TAG_RE.match(tag)
      if match:
        level = int(match.group(1))
        self._generator.StartHeader(level)

  def endElementNS(self, name, qname):
    tag = name[1]
    if tag == 'a':
      self._generator.EndAnchor()
    elif tag == 'li':
      self._generator.EndListItem()
    elif tag == 'div':
      self._generator.EndDiv()
    elif tag == 'p':
      self._generator.EndParagraph()
    elif tag in ('b', 'code', 'em', 'i', 'strong', 's', 'strike', 'del', 'u'):
      self._generator.EndFormat()
    elif tag in ('ul', 'ol'):
      self._generator.EndList()
    elif tag == 'blockquote':
      self._generator.EndBlockquote()
    elif tag == 'td':
      self._generator.EndTD()
    elif tag == 'tr':
      self._generator.EndTR()
    elif tag == 'table':
      self._generator.EndTable()
    else:
      match = self._HEADER_TAG_RE.match(tag)
      if match:
        self._generator.EndHeader()

  def characters(self, content):
    self._generator.Text(content)


class DefaultUrlTranslator:
  """No-op UrlTranslator."""

  def Translate(self, href):
    return href


def Convert(input_stream, output_stream, url_translator=DefaultUrlTranslator()):
  """Converts an input stream of xhtml into an output stream of markdown.

  Args:
     input_stream: filehandle for the XHTML input.
     output_stream: filehandle for the Markdown output.
     url_translator: Callback for translating URLs embedded in the page.
  """
  parser = xml.sax.make_parser()
  parser.setContentHandler(XhtmlHandler(output_stream, url_translator))
  parser.setFeature(xml.sax.handler.feature_namespaces, 1)
  parser.parse(input_stream)
