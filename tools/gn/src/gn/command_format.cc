// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/command_format.h"

#include <stddef.h>

#include <sstream>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "gn/commands.h"
#include "gn/filesystem_utils.h"
#include "gn/input_file.h"
#include "gn/parser.h"
#include "gn/scheduler.h"
#include "gn/setup.h"
#include "gn/source_file.h"
#include "gn/string_utils.h"
#include "gn/switches.h"
#include "gn/tokenizer.h"
#include "util/build_config.h"

#if defined(OS_WIN)
#include <fcntl.h>
#include <io.h>
#endif

namespace commands {

const char kSwitchDryRun[] = "dry-run";
const char kSwitchDumpTree[] = "dump-tree";
const char kSwitchReadTree[] = "read-tree";
const char kSwitchStdin[] = "stdin";
const char kSwitchTreeTypeJSON[] = "json";
const char kSwitchTreeTypeText[] = "text";

const char kFormat[] = "format";
const char kFormat_HelpShort[] = "format: Format .gn files.";
const char kFormat_Help[] =
    R"(gn format [--dump-tree] (--stdin | <list of build_files...>)

  Formats .gn file to a standard format.

  The contents of some lists ('sources', 'deps', etc.) will be sorted to a
  canonical order. To suppress this, you can add a comment of the form "#
  NOSORT" immediately preceding the assignment. e.g.

  # NOSORT
  sources = [
    "z.cc",
    "a.cc",
  ]

Arguments

  --dry-run
      Prints the list of files that would be reformatted but does not write
      anything to disk. This is useful for presubmit/lint-type checks.
      - Exit code 0: successful format, matches on disk.
      - Exit code 1: general failure (parse error, etc.)
      - Exit code 2: successful format, but differs from on disk.

  --dump-tree[=( text | json )]
      Dumps the parse tree to stdout and does not update the file or print
      formatted output. If no format is specified, text format will be used.

  --stdin
      Read input from stdin and write to stdout rather than update a file
      in-place.

  --read-tree=json
      Reads an AST from stdin in the format output by --dump-tree=json and
      uses that as the parse tree. (The only read-tree format currently
      supported is json.) The given .gn file will be overwritten. This can be
      used to programmatically transform .gn files.

Examples
  gn format //some/BUILD.gn //some/other/BUILD.gn //and/another/BUILD.gn
  gn format some\\BUILD.gn
  gn format /abspath/some/BUILD.gn
  gn format --stdin
  gn format --read-tree=json //rewritten/BUILD.gn
)";

namespace {

const int kIndentSize = 2;
const int kMaximumWidth = 80;

const int kPenaltyLineBreak = 500;
const int kPenaltyHorizontalSeparation = 100;
const int kPenaltyExcess = 10000;
const int kPenaltyBrokenLineOnOneLiner = 5000;

enum Precedence {
  kPrecedenceLowest,
  kPrecedenceAssign,
  kPrecedenceOr,
  kPrecedenceAnd,
  kPrecedenceCompare,
  kPrecedenceAdd,
  kPrecedenceUnary,
  kPrecedenceSuffix,
};

int CountLines(const std::string& str) {
  return static_cast<int>(base::SplitStringPiece(str, "\n",
                                                 base::KEEP_WHITESPACE,
                                                 base::SPLIT_WANT_ALL)
                              .size());
}

class Printer {
 public:
  Printer();
  ~Printer();

  void Block(const ParseNode* file);

  std::string String() const { return output_; }

 private:
  // Format a list of values using the given style.
  enum SequenceStyle {
    kSequenceStyleList,
    kSequenceStyleBracedBlock,
    kSequenceStyleBracedBlockAlreadyOpen,
  };

  struct Metrics {
    Metrics() : first_length(-1), longest_length(-1), multiline(false) {}
    int first_length;
    int longest_length;
    bool multiline;
  };

  // Add to output.
  void Print(std::string_view str);

  // Add the current margin (as spaces) to the output.
  void PrintMargin();

  void TrimAndPrintToken(const Token& token);

  void PrintTrailingCommentsWrapped(const std::vector<Token>& comments);

  void FlushComments();

  void PrintSuffixComments(const ParseNode* node);

  // End the current line, flushing end of line comments.
  void Newline();

  // Remove trailing spaces from the current line.
  void Trim();

  // Whether there's a blank separator line at the current position.
  bool HaveBlankLine();

  // Sort a list on the RHS if the LHS is one of the following:
  // 'sources': sorted alphabetically.
  // 'deps' or ends in 'deps': sorted such that relative targets are first,
  //   followed by global targets, each internally sorted alphabetically.
  // 'visibility': same as 'deps'.
  void SortIfApplicable(const BinaryOpNode* binop);

  // Traverse a binary op node tree and apply a callback to each leaf node.
  void TraverseBinaryOpNode(const ParseNode* node,
                            std::function<void(const ParseNode*)> callback);

  // Sort contiguous import() function calls in the given ordered list of
  // statements (the body of a block or scope).
  template <class PARSENODE>
  void SortImports(std::vector<std::unique_ptr<PARSENODE>>& statements);

  // Heuristics to decide if there should be a blank line added between two
  // items. For various "small" items, it doesn't look nice if there's too much
  // vertical whitespace added.
  bool ShouldAddBlankLineInBetween(const ParseNode* a, const ParseNode* b);

  // Get the 0-based x position on the current line.
  int CurrentColumn() const;

  // Get the current line in the output;
  int CurrentLine() const;

  // Adds an opening ( if prec is less than the outers (to maintain evalution
  // order for a subexpression). If an opening paren is emitted, *parenthesized
  // will be set so it can be closed at the end of the expression.
  void AddParen(int prec, int outer_prec, bool* parenthesized);

  // Print the expression given by |root| to the output buffer and appends
  // |suffix| to that output. Returns a penalty that represents the cost of
  // adding that output to the buffer (where higher is worse). The value of
  // outer_prec gives the precedence of the operator outside this Expr. If that
  // operator binds tighter than root's, Expr() must introduce parentheses.
  int Expr(const ParseNode* root, int outer_prec, const std::string& suffix);

  // Generic penalties for exceeding maximum width, adding more lines, etc.
  int AssessPenalty(const std::string& output);

  // Tests if any lines exceed the maximum width.
  bool ExceedsMaximumWidth(const std::string& output);

  // Format a list of values using the given style.
  // |end| holds any trailing comments to be printed just before the closing
  // bracket.
  template <class PARSENODE>  // Just for const covariance.
  void Sequence(SequenceStyle style,
                const std::vector<std::unique_ptr<PARSENODE>>& list,
                const ParseNode* end,
                bool force_multiline);

  // Returns the penalty.
  int FunctionCall(const FunctionCallNode* func_call,
                   const std::string& suffix);

  // Create a clone of this Printer in a similar state (other than the output,
  // but including margins, etc.) to be used for dry run measurements.
  void InitializeSub(Printer* sub);

  template <class PARSENODE>
  bool ListWillBeMultiline(const std::vector<std::unique_ptr<PARSENODE>>& list,
                           const ParseNode* end);

  std::string output_;           // Output buffer.
  std::vector<Token> comments_;  // Pending end-of-line comments.
  int margin() const { return stack_.back().margin; }

  int penalty_depth_;
  int GetPenaltyForLineBreak() const {
    return penalty_depth_ * kPenaltyLineBreak;
  }

  struct IndentState {
    IndentState()
        : margin(0),
          continuation_requires_indent(false),
          parent_is_boolean_or(false) {}
    IndentState(int margin,
                bool continuation_requires_indent,
                bool parent_is_boolean_or)
        : margin(margin),
          continuation_requires_indent(continuation_requires_indent),
          parent_is_boolean_or(parent_is_boolean_or) {}

    // The left margin (number of spaces).
    int margin;

    bool continuation_requires_indent;

    bool parent_is_boolean_or;
  };
  // Stack used to track
  std::vector<IndentState> stack_;

  // Gives the precedence for operators in a BinaryOpNode.
  std::map<std::string_view, Precedence> precedence_;

  Printer(const Printer&) = delete;
  Printer& operator=(const Printer&) = delete;
};

Printer::Printer() : penalty_depth_(0) {
  output_.reserve(100 << 10);
  precedence_["="] = kPrecedenceAssign;
  precedence_["+="] = kPrecedenceAssign;
  precedence_["-="] = kPrecedenceAssign;
  precedence_["||"] = kPrecedenceOr;
  precedence_["&&"] = kPrecedenceAnd;
  precedence_["<"] = kPrecedenceCompare;
  precedence_[">"] = kPrecedenceCompare;
  precedence_["=="] = kPrecedenceCompare;
  precedence_["!="] = kPrecedenceCompare;
  precedence_["<="] = kPrecedenceCompare;
  precedence_[">="] = kPrecedenceCompare;
  precedence_["+"] = kPrecedenceAdd;
  precedence_["-"] = kPrecedenceAdd;
  precedence_["!"] = kPrecedenceUnary;
  stack_.push_back(IndentState());
}

Printer::~Printer() = default;

void Printer::Print(std::string_view str) {
  output_.append(str);
}

void Printer::PrintMargin() {
  output_ += std::string(margin(), ' ');
}

void Printer::TrimAndPrintToken(const Token& token) {
  std::string trimmed;
  TrimWhitespaceASCII(std::string(token.value()), base::TRIM_ALL, &trimmed);
  Print(trimmed);
}

// Assumes that the margin is set to the indent level where the comments should
// be aligned. This doesn't de-wrap, it only wraps. So if a suffix comment
// causes the line to exceed 80 col it will be wrapped, but the subsequent line
// would fit on the then-broken line it will not be merged with it. This is
// partly because it's difficult to implement at this level, but also because
// it can break hand-authored line breaks where they're starting a new paragraph
// or statement.
void Printer::PrintTrailingCommentsWrapped(const std::vector<Token>& comments) {
  bool have_empty_line = true;
  auto start_next_line = [this, &have_empty_line]() {
    Trim();
    Print("\n");
    PrintMargin();
    have_empty_line = true;
  };
  for (const auto& c : comments) {
    if (!have_empty_line) {
      start_next_line();
    }

    std::string trimmed;
    TrimWhitespaceASCII(std::string(c.value()), base::TRIM_ALL, &trimmed);

    if (margin() + trimmed.size() <= kMaximumWidth) {
      Print(trimmed);
      have_empty_line = false;
    } else {
      bool continuation = false;
      std::vector<std::string> split_on_spaces = base::SplitString(
          c.value(), " ", base::WhitespaceHandling::TRIM_WHITESPACE,
          base::SplitResult::SPLIT_WANT_NONEMPTY);
      for (size_t j = 0; j < split_on_spaces.size(); ++j) {
        if (have_empty_line && continuation) {
          Print("# ");
        }
        Print(split_on_spaces[j]);
        Print(" ");
        if (split_on_spaces[j] != "#") {
          have_empty_line = false;
        }
        if (!have_empty_line &&
            (j < split_on_spaces.size() - 1 &&
             CurrentColumn() + split_on_spaces[j + 1].size() > kMaximumWidth)) {
          start_next_line();
          continuation = true;
        }
      }
    }
  }
}

// Used during penalty evaluation, similar to Newline().
void Printer::PrintSuffixComments(const ParseNode* node) {
  if (node->comments() && !node->comments()->suffix().empty()) {
    Print("  ");
    stack_.push_back(IndentState(CurrentColumn(), false, false));
    PrintTrailingCommentsWrapped(node->comments()->suffix());
    stack_.pop_back();
  }
}

void Printer::FlushComments() {
  if (!comments_.empty()) {
    Print("  ");
    // Save the margin, and temporarily set it to where the first comment
    // starts so that multiple suffix comments are vertically aligned.
    stack_.push_back(IndentState(CurrentColumn(), false, false));
    PrintTrailingCommentsWrapped(comments_);
    stack_.pop_back();
    comments_.clear();
  }
}

void Printer::Newline() {
  FlushComments();
  Trim();
  Print("\n");
  PrintMargin();
}

void Printer::Trim() {
  size_t n = output_.size();
  while (n > 0 && output_[n - 1] == ' ')
    --n;
  output_.resize(n);
}

bool Printer::HaveBlankLine() {
  size_t n = output_.size();
  while (n > 0 && output_[n - 1] == ' ')
    --n;
  return n > 2 && output_[n - 1] == '\n' && output_[n - 2] == '\n';
}

void Printer::SortIfApplicable(const BinaryOpNode* binop) {
  if (const Comments* comments = binop->comments()) {
    const std::vector<Token>& before = comments->before();
    if (!before.empty() && (before.front().value() == "# NOSORT" ||
                            before.back().value() == "# NOSORT")) {
      // Allow disabling of sort for specific actions that might be
      // order-sensitive.
      return;
    }
  }
  const IdentifierNode* ident = binop->left()->AsIdentifier();
  if ((binop->op().value() == "=" || binop->op().value() == "+=" ||
       binop->op().value() == "-=") &&
      ident) {
    const std::string_view lhs = ident->value().value();
    if (base::EndsWith(lhs, "sources", base::CompareCase::SENSITIVE) ||
        lhs == "public") {
      TraverseBinaryOpNode(binop->right(), [](const ParseNode* node) {
        const ListNode* list = node->AsList();
        if (list)
          const_cast<ListNode*>(list)->SortAsStringsList();
      });
    } else if (base::EndsWith(lhs, "deps", base::CompareCase::SENSITIVE) ||
               lhs == "visibility") {
      TraverseBinaryOpNode(binop->right(), [](const ParseNode* node) {
        const ListNode* list = node->AsList();
        if (list)
          const_cast<ListNode*>(list)->SortAsTargetsList();
      });
    }
  }
}

void Printer::TraverseBinaryOpNode(
    const ParseNode* node,
    std::function<void(const ParseNode*)> callback) {
  const BinaryOpNode* binop = node->AsBinaryOp();
  if (binop) {
    TraverseBinaryOpNode(binop->left(), callback);
    TraverseBinaryOpNode(binop->right(), callback);
  } else {
    callback(node);
  }
}

template <class PARSENODE>
void Printer::SortImports(std::vector<std::unique_ptr<PARSENODE>>& statements) {
  // Build a set of ranges by indices of FunctionCallNode's that are imports.

  std::vector<std::vector<size_t>> import_statements;

  auto is_import = [](const PARSENODE* p) {
    const FunctionCallNode* func_call = p->AsFunctionCall();
    return func_call && func_call->function().value() == "import";
  };

  std::vector<size_t> current_group;
  for (size_t i = 0; i < statements.size(); ++i) {
    if (is_import(statements[i].get())) {
      if (i > 0 && (!is_import(statements[i - 1].get()) ||
                    ShouldAddBlankLineInBetween(statements[i - 1].get(),
                                                statements[i].get()))) {
        if (!current_group.empty()) {
          import_statements.push_back(current_group);
          current_group.clear();
        }
      }
      current_group.push_back(i);
    }
  }

  if (!current_group.empty())
    import_statements.push_back(current_group);

  struct CompareByImportFile {
    bool operator()(const std::unique_ptr<PARSENODE>& a,
                    const std::unique_ptr<PARSENODE>& b) const {
      const auto& a_args = a->AsFunctionCall()->args()->contents();
      const auto& b_args = b->AsFunctionCall()->args()->contents();
      std::string_view a_name;
      std::string_view b_name;

      // Non-literal imports are treated as empty names, and order is
      // maintained. Arbitrarily complex expressions in import() are
      // rare, and it probably doesn't make sense to sort non-string
      // literals anyway, see format_test_data/083.gn.
      if (!a_args.empty() && a_args[0]->AsLiteral())
        a_name = a_args[0]->AsLiteral()->value().value();
      if (!b_args.empty() && b_args[0]->AsLiteral())
        b_name = b_args[0]->AsLiteral()->value().value();

      auto is_absolute = [](std::string_view import) {
        return import.size() >= 3 && import[0] == '"' && import[1] == '/' &&
               import[2] == '/';
      };
      int a_is_rel = !is_absolute(a_name);
      int b_is_rel = !is_absolute(b_name);

      return std::tie(a_is_rel, a_name) < std::tie(b_is_rel, b_name);
    }
  };

  int line_after_previous = -1;

  for (const auto& group : import_statements) {
    size_t begin = group[0];
    size_t end = group.back() + 1;

    // Save the original line number so that ranges can be re-assigned. They're
    // contiguous because of the partitioning code above. Later formatting
    // relies on correct line number to know whether to insert blank lines,
    // which is why these need to be fixed up. Additionally, to handle multiple
    // imports on one line, they're assigned sequential line numbers, and
    // subsequent blocks will be gapped from them.
    int start_line =
        std::max(statements[begin]->GetRange().begin().line_number(),
                 line_after_previous + 1);

    std::sort(statements.begin() + begin, statements.begin() + end,
              CompareByImportFile());

    const PARSENODE* prev = nullptr;
    for (size_t i = begin; i < end; ++i) {
      const PARSENODE* node = statements[i].get();
      int line_number =
          prev ? prev->GetRange().end().line_number() + 1 : start_line;
      if (node->comments() && !node->comments()->before().empty())
        line_number++;
      const_cast<FunctionCallNode*>(node->AsFunctionCall())
          ->SetNewLocation(line_number);
      prev = node;
      line_after_previous = line_number + 1;
    }
  }
}

namespace {

int SuffixCommentTreeWalk(const ParseNode* node) {
  // Check all the children for suffix comments. This is conceptually simple,
  // but ugly as there's not a generic parse tree walker. This walker goes
  // lowest child first so that if it's valid that's returned.
  if (!node)
    return -1;

#define RETURN_IF_SET(x)             \
  if (int result = (x); result >= 0) \
    return result

  if (const AccessorNode* accessor = node->AsAccessor()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(accessor->subscript()));
    RETURN_IF_SET(SuffixCommentTreeWalk(accessor->member()));
  } else if (const BinaryOpNode* binop = node->AsBinaryOp()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(binop->right()));
  } else if (const BlockNode* block = node->AsBlock()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(block->End()));
  } else if (const ConditionNode* condition = node->AsCondition()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(condition->if_false()));
    RETURN_IF_SET(SuffixCommentTreeWalk(condition->if_true()));
    RETURN_IF_SET(SuffixCommentTreeWalk(condition->condition()));
  } else if (const FunctionCallNode* func_call = node->AsFunctionCall()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(func_call->block()));
    RETURN_IF_SET(SuffixCommentTreeWalk(func_call->args()));
  } else if (node->AsIdentifier()) {
    // Nothing.
  } else if (const ListNode* list = node->AsList()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(list->End()));
  } else if (node->AsLiteral()) {
    // Nothing.
  } else if (const UnaryOpNode* unaryop = node->AsUnaryOp()) {
    RETURN_IF_SET(SuffixCommentTreeWalk(unaryop->operand()));
  } else if (node->AsBlockComment()) {
    // Nothing.
  } else if (node->AsEnd()) {
    // Nothing.
  } else {
    CHECK(false) << "Unhandled case in SuffixCommentTreeWalk.";
  }

#undef RETURN_IF_SET

  // Check this node if there are no child comments.
  if (node->comments() && !node->comments()->suffix().empty()) {
    return node->comments()->suffix().back().location().line_number();
  }

  return -1;
}

// If there are suffix comments on the first node or its children, they might
// carry down multiple lines. Otherwise, use the node's normal end range. This
// function is needed because the parse tree doesn't include comments in the
// location ranges, and it's not a straightforword change to add them. So this
// is effectively finding the "real" range for |root| including suffix comments.
// Note that it's not enough to simply look at |root|'s suffix comments because
// in the case of:
//
//   a =
//       b + c  # something
//              # or other
//   x = y
//
// the comments are attached to a BinOp+ which is a child of BinOp=, not
// directly to the BinOp= which will be what's being used to determine if there
// should be a blank line inserted before the |x| line.
int FindLowestSuffixComment(const ParseNode* root) {
  LocationRange range = root->GetRange();
  int end = range.end().line_number();
  int result = SuffixCommentTreeWalk(root);
  return (result == -1 || result < end) ? end : result;
}

}  // namespace

bool Printer::ShouldAddBlankLineInBetween(const ParseNode* a,
                                          const ParseNode* b) {
  LocationRange b_range = b->GetRange();
  int a_end = FindLowestSuffixComment(a);

  // If they're already separated by 1 or more lines, then we want to keep a
  // blank line.
  return (b_range.begin().line_number() > a_end + 1) ||
         // Always put a blank line before a block comment.
         b->AsBlockComment();
}

int Printer::CurrentColumn() const {
  int n = 0;
  while (n < static_cast<int>(output_.size()) &&
         output_[output_.size() - 1 - n] != '\n') {
    ++n;
  }
  return n;
}

int Printer::CurrentLine() const {
  int count = 1;
  for (const char* p = output_.c_str(); (p = strchr(p, '\n')) != nullptr;) {
    ++count;
    ++p;
  }
  return count;
}

void Printer::Block(const ParseNode* root) {
  const BlockNode* block = root->AsBlock();

  if (block->comments()) {
    for (const auto& c : block->comments()->before()) {
      TrimAndPrintToken(c);
      Newline();
    }
  }

  SortImports(const_cast<std::vector<std::unique_ptr<ParseNode>>&>(
      block->statements()));

  size_t i = 0;
  for (const auto& stmt : block->statements()) {
    Expr(stmt.get(), kPrecedenceLowest, std::string());
    Newline();
    if (stmt->comments()) {
      // Why are before() not printed here too? before() are handled inside
      // Expr(), as are suffix() which are queued to the next Newline().
      // However, because it's a general expression handler, it doesn't insert
      // the newline itself, which only happens between block statements. So,
      // the after are handled explicitly here.
      for (const auto& c : stmt->comments()->after()) {
        TrimAndPrintToken(c);
        Newline();
      }
    }
    if (i < block->statements().size() - 1 &&
        (ShouldAddBlankLineInBetween(block->statements()[i].get(),
                                     block->statements()[i + 1].get()))) {
      Newline();
    }
    ++i;
  }

  if (block->comments()) {
    if (!block->statements().empty() &&
        block->statements().back()->AsBlockComment()) {
      // If the block ends in a comment, and there's a comment following it,
      // then the two comments were originally separate, so keep them that way.
      Newline();
    }
    for (const auto& c : block->comments()->after()) {
      TrimAndPrintToken(c);
      Newline();
    }
  }
}

int Printer::AssessPenalty(const std::string& output) {
  int penalty = 0;
  std::vector<std::string> lines = base::SplitString(
      output, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  penalty += static_cast<int>(lines.size() - 1) * GetPenaltyForLineBreak();
  for (const auto& line : lines) {
    if (line.size() > kMaximumWidth)
      penalty += static_cast<int>(line.size() - kMaximumWidth) * kPenaltyExcess;
  }
  return penalty;
}

bool Printer::ExceedsMaximumWidth(const std::string& output) {
  for (const auto& line : base::SplitString(output, "\n", base::KEEP_WHITESPACE,
                                            base::SPLIT_WANT_ALL)) {
    std::string_view trimmed =
        TrimString(line, " ", base::TrimPositions::TRIM_TRAILING);
    if (trimmed.size() > kMaximumWidth) {
      return true;
    }
  }
  return false;
}

void Printer::AddParen(int prec, int outer_prec, bool* parenthesized) {
  if (prec < outer_prec) {
    Print("(");
    *parenthesized = true;
  }
}

int Printer::Expr(const ParseNode* root,
                  int outer_prec,
                  const std::string& suffix) {
  std::string at_end = suffix;
  int penalty = 0;
  penalty_depth_++;

  if (root->comments()) {
    if (!root->comments()->before().empty()) {
      Trim();
      // If there's already other text on the line, start a new line.
      if (CurrentColumn() > 0)
        Print("\n");
      // We're printing a line comment, so we need to be at the current margin.
      PrintMargin();
      for (const auto& c : root->comments()->before()) {
        TrimAndPrintToken(c);
        Newline();
      }
    }
  }

  bool parenthesized = false;

  if (const AccessorNode* accessor = root->AsAccessor()) {
    AddParen(kPrecedenceSuffix, outer_prec, &parenthesized);
    Print(accessor->base().value());
    if (accessor->member()) {
      Print(".");
      Expr(accessor->member(), kPrecedenceLowest, std::string());
    } else {
      CHECK(accessor->subscript());
      Print("[");
      Expr(accessor->subscript(), kPrecedenceLowest, "]");
    }
  } else if (const BinaryOpNode* binop = root->AsBinaryOp()) {
    CHECK(precedence_.find(binop->op().value()) != precedence_.end());

    SortIfApplicable(binop);

    Precedence prec = precedence_[binop->op().value()];

    // Since binary operators format left-to-right, it is ok for the left side
    // use the same operator without parentheses, so the left uses prec. For the
    // same reason, the right side cannot reuse the same operator, or else "x +
    // (y + z)" would format as "x + y + z" which means "(x + y) + z". So, treat
    // the right expression as appearing one precedence level higher.
    // However, because the source parens are not in the parse tree, as a
    // special case for && and || we insert strictly-redundant-but-helpful-for-
    // human-readers parentheses.
    int prec_left = prec;
    int prec_right = prec + 1;
    if (binop->op().value() == "&&" && stack_.back().parent_is_boolean_or) {
      Print("(");
      parenthesized = true;
    } else {
      AddParen(prec_left, outer_prec, &parenthesized);
    }

    if (parenthesized)
      at_end = ")" + at_end;

    int start_line = CurrentLine();
    int start_column = CurrentColumn();
    bool is_assignment = binop->op().value() == "=" ||
                         binop->op().value() == "+=" ||
                         binop->op().value() == "-=";

    int indent_column = start_column;
    if (is_assignment) {
      // Default to a double-indent for wrapped assignments.
      indent_column = margin() + kIndentSize * 2;

      // A special case for the long lists and scope assignments that are
      // common in .gn files, don't indent them + 4, even though they're just
      // continuations when they're simple lists like "x = [ a, b, c, ... ]" or
      // scopes like "x = { a = 1 b = 2 }". Put back to "normal" indenting.
      if (const ListNode* right_as_list = binop->right()->AsList()) {
        if (ListWillBeMultiline(right_as_list->contents(),
                                right_as_list->End()))
          indent_column = start_column;
      } else {
        if (binop->right()->AsBlock())
          indent_column = start_column;
      }
    }
    if (stack_.back().continuation_requires_indent)
      indent_column += kIndentSize * 2;

    stack_.push_back(IndentState(indent_column,
                                 stack_.back().continuation_requires_indent,
                                 binop->op().value() == "||"));
    Printer sub_left;
    InitializeSub(&sub_left);
    sub_left.Expr(binop->left(), prec_left,
                  std::string(" ") + std::string(binop->op().value()));
    bool left_is_multiline = CountLines(sub_left.String()) > 1;
    // Avoid walking the whole left redundantly times (see timing of Format.046)
    // so pull the output and comments from subprinter.
    Print(sub_left.String().substr(start_column));
    std::copy(sub_left.comments_.begin(), sub_left.comments_.end(),
              std::back_inserter(comments_));

    // Single line.
    Printer sub1;
    InitializeSub(&sub1);
    sub1.Print(" ");
    int penalty_current_line = sub1.Expr(binop->right(), prec_right, at_end);
    sub1.PrintSuffixComments(root);
    sub1.FlushComments();
    penalty_current_line += AssessPenalty(sub1.String());
    if (!is_assignment && left_is_multiline) {
      // In e.g. xxx + yyy, if xxx is already multiline, then we want a penalty
      // for trying to continue as if this were one line.
      penalty_current_line +=
          (CountLines(sub1.String()) - 1) * kPenaltyBrokenLineOnOneLiner;
    }

    // Break after operator.
    Printer sub2;
    InitializeSub(&sub2);
    sub2.Newline();
    int penalty_next_line = sub2.Expr(binop->right(), prec_right, at_end);
    sub2.PrintSuffixComments(root);
    sub2.FlushComments();
    penalty_next_line += AssessPenalty(sub2.String());

    // Force a list on the RHS that would normally be a single line into
    // multiline.
    bool tried_rhs_multiline = false;
    Printer sub3;
    InitializeSub(&sub3);
    int penalty_multiline_rhs_list = std::numeric_limits<int>::max();
    const ListNode* rhs_list = binop->right()->AsList();
    if (is_assignment && rhs_list &&
        !ListWillBeMultiline(rhs_list->contents(), rhs_list->End())) {
      sub3.Print(" ");
      sub3.stack_.push_back(IndentState(start_column, false, false));
      sub3.Sequence(kSequenceStyleList, rhs_list->contents(), rhs_list->End(),
                    true);
      sub3.PrintSuffixComments(root);
      sub3.FlushComments();
      sub3.stack_.pop_back();
      penalty_multiline_rhs_list = AssessPenalty(sub3.String());
      tried_rhs_multiline = true;
    }

    // If in all cases it was forced past 80col, then we don't break to avoid
    // breaking after '=' in the case of:
    //   variable = "... very long string ..."
    // as breaking and indenting doesn't make things much more readable, even
    // though there's fewer characters past the maximum width.
    bool exceeds_maximum_all_ways =
        ExceedsMaximumWidth(sub1.String()) &&
        ExceedsMaximumWidth(sub2.String()) &&
        (!tried_rhs_multiline || ExceedsMaximumWidth(sub3.String()));

    if (penalty_current_line < penalty_next_line || exceeds_maximum_all_ways) {
      Print(" ");
      Expr(binop->right(), prec_right, at_end);
      at_end = "";
    } else if (tried_rhs_multiline &&
               penalty_multiline_rhs_list < penalty_next_line) {
      // Force a multiline list on the right.
      Print(" ");
      stack_.push_back(IndentState(start_column, false, false));
      Sequence(kSequenceStyleList, rhs_list->contents(), rhs_list->End(), true);
      stack_.pop_back();
    } else {
      // Otherwise, put first argument and op, and indent next.
      Newline();
      penalty += std::abs(CurrentColumn() - start_column) *
                 kPenaltyHorizontalSeparation;
      Expr(binop->right(), prec_right, at_end);
      at_end = "";
    }
    stack_.pop_back();
    penalty += (CurrentLine() - start_line) * GetPenaltyForLineBreak();
  } else if (const BlockNode* block = root->AsBlock()) {
    Sequence(kSequenceStyleBracedBlock, block->statements(), block->End(),
             false);
  } else if (const ConditionNode* condition = root->AsCondition()) {
    Print("if (");
    CHECK(at_end.empty());
    Expr(condition->condition(), kPrecedenceLowest, ") {");
    Sequence(kSequenceStyleBracedBlockAlreadyOpen,
             condition->if_true()->statements(), condition->if_true()->End(),
             false);
    if (condition->if_false()) {
      Print(" else ");
      // If it's a block it's a bare 'else', otherwise it's an 'else if'. See
      // ConditionNode::Execute.
      bool is_else_if = condition->if_false()->AsBlock() == nullptr;
      if (is_else_if) {
        Expr(condition->if_false(), kPrecedenceLowest, std::string());
      } else {
        Sequence(kSequenceStyleBracedBlock,
                 condition->if_false()->AsBlock()->statements(),
                 condition->if_false()->AsBlock()->End(), false);
      }
    }
  } else if (const FunctionCallNode* func_call = root->AsFunctionCall()) {
    penalty += FunctionCall(func_call, at_end);
    at_end = "";
  } else if (const IdentifierNode* identifier = root->AsIdentifier()) {
    Print(identifier->value().value());
  } else if (const ListNode* list = root->AsList()) {
    Sequence(kSequenceStyleList, list->contents(), list->End(),
             /*force_multiline=*/false);
  } else if (const LiteralNode* literal = root->AsLiteral()) {
    Print(literal->value().value());
  } else if (const UnaryOpNode* unaryop = root->AsUnaryOp()) {
    Print(unaryop->op().value());
    Expr(unaryop->operand(), kPrecedenceUnary, std::string());
  } else if (const BlockCommentNode* block_comment = root->AsBlockComment()) {
    Print(block_comment->comment().value());
  } else if (const EndNode* end = root->AsEnd()) {
    Print(end->value().value());
  } else {
    CHECK(false) << "Unhandled case in Expr.";
  }

  // Defer any end of line comment until we reach the newline.
  if (root->comments() && !root->comments()->suffix().empty()) {
    std::copy(root->comments()->suffix().begin(),
              root->comments()->suffix().end(), std::back_inserter(comments_));
  }

  Print(at_end);

  penalty_depth_--;
  return penalty;
}

template <class PARSENODE>
void Printer::Sequence(SequenceStyle style,
                       const std::vector<std::unique_ptr<PARSENODE>>& list,
                       const ParseNode* end,
                       bool force_multiline) {
  if (style == kSequenceStyleList) {
    Print("[");
  } else if (style == kSequenceStyleBracedBlock) {
    Print("{");
  } else if (style == kSequenceStyleBracedBlockAlreadyOpen) {
    style = kSequenceStyleBracedBlock;
  }

  if (style == kSequenceStyleBracedBlock) {
    force_multiline = true;
    SortImports(const_cast<std::vector<std::unique_ptr<PARSENODE>>&>(list));
  }

  force_multiline |= ListWillBeMultiline(list, end);

  if (list.size() == 0 && !force_multiline) {
    // No elements, and not forcing newlines, print nothing.
  } else if (list.size() == 1 && !force_multiline) {
    Print(" ");
    Expr(list[0].get(), kPrecedenceLowest, std::string());
    CHECK(!list[0]->comments() || list[0]->comments()->after().empty());
    Print(" ");
  } else {
    stack_.push_back(IndentState(margin() + kIndentSize,
                                 style == kSequenceStyleList, false));
    size_t i = 0;
    for (const auto& x : list) {
      Newline();
      // If:
      // - we're going to output some comments, and;
      // - we haven't just started this multiline list, and;
      // - there isn't already a blank line here;
      // Then: insert one.
      if (i != 0 && x->comments() && !x->comments()->before().empty() &&
          !HaveBlankLine()) {
        Newline();
      }
      bool body_of_list = i < list.size() - 1 || style == kSequenceStyleList;
      bool want_comma =
          body_of_list && (style == kSequenceStyleList && !x->AsBlockComment());
      Expr(x.get(), kPrecedenceLowest, want_comma ? "," : std::string());
      CHECK(!x->comments() || x->comments()->after().empty());
      if (body_of_list) {
        if (i < list.size() - 1 &&
            ShouldAddBlankLineInBetween(list[i].get(), list[i + 1].get()))
          Newline();
      }
      ++i;
    }

    // Trailing comments.
    if (end->comments() && !end->comments()->before().empty()) {
      if (list.size() >= 2)
        Newline();
      for (const auto& c : end->comments()->before()) {
        Newline();
        TrimAndPrintToken(c);
      }
    }

    stack_.pop_back();
    Newline();
  }

  // Defer any end of line comment until we reach the newline.
  if (end->comments() && !end->comments()->suffix().empty()) {
    std::copy(end->comments()->suffix().begin(),
              end->comments()->suffix().end(), std::back_inserter(comments_));
  }

  if (style == kSequenceStyleList)
    Print("]");
  else if (style == kSequenceStyleBracedBlock)
    Print("}");
}

int Printer::FunctionCall(const FunctionCallNode* func_call,
                          const std::string& suffix) {
  int start_line = CurrentLine();
  int start_column = CurrentColumn();
  Print(func_call->function().value());
  Print("(");

  bool have_block = func_call->block() != nullptr;
  bool force_multiline = false;

  const auto& list = func_call->args()->contents();
  const ParseNode* end = func_call->args()->End();

  if (end->comments() && !end->comments()->before().empty())
    force_multiline = true;

  // If there's before line comments, make sure we have a place to put them.
  for (const auto& i : list) {
    if (i->comments() && !i->comments()->before().empty())
      force_multiline = true;
  }

  // Calculate the penalties for 3 possible layouts:
  // 1. all on same line;
  // 2. starting on same line, broken at each comma but paren aligned;
  // 3. broken to next line + 4, broken at each comma.
  std::string terminator = ")";
  if (have_block)
    terminator += " {";
  terminator += suffix;

  // Special case to make function calls of one arg taking a long list of
  // boolean operators not indent.
  bool continuation_requires_indent =
      list.size() != 1 || !list[0]->AsBinaryOp();

  // 1: Same line.
  Printer sub1;
  InitializeSub(&sub1);
  sub1.stack_.push_back(
      IndentState(CurrentColumn(), continuation_requires_indent, false));
  int penalty_one_line = 0;
  for (size_t i = 0; i < list.size(); ++i) {
    penalty_one_line += sub1.Expr(list[i].get(), kPrecedenceLowest,
                                  i < list.size() - 1 ? ", " : std::string());
  }
  sub1.Print(terminator);
  penalty_one_line += AssessPenalty(sub1.String());
  // This extra penalty prevents a short second argument from being squeezed in
  // after a first argument that went multiline (and instead preferring a
  // variant below).
  penalty_one_line +=
      (CountLines(sub1.String()) - 1) * kPenaltyBrokenLineOnOneLiner;

  // 2: Starting on same line, broken at commas.
  Printer sub2;
  InitializeSub(&sub2);
  sub2.stack_.push_back(
      IndentState(CurrentColumn(), continuation_requires_indent, false));
  int penalty_multiline_start_same_line = 0;
  for (size_t i = 0; i < list.size(); ++i) {
    penalty_multiline_start_same_line +=
        sub2.Expr(list[i].get(), kPrecedenceLowest,
                  i < list.size() - 1 ? "," : std::string());
    if (i < list.size() - 1) {
      sub2.Newline();
    }
  }
  sub2.Print(terminator);
  penalty_multiline_start_same_line += AssessPenalty(sub2.String());

  // 3: Starting on next line, broken at commas.
  Printer sub3;
  InitializeSub(&sub3);
  sub3.stack_.push_back(IndentState(margin() + kIndentSize * 2,
                                    continuation_requires_indent, false));
  sub3.Newline();
  int penalty_multiline_start_next_line = 0;
  for (size_t i = 0; i < list.size(); ++i) {
    if (i == 0) {
      penalty_multiline_start_next_line +=
          std::abs(sub3.CurrentColumn() - start_column) *
          kPenaltyHorizontalSeparation;
    }
    penalty_multiline_start_next_line +=
        sub3.Expr(list[i].get(), kPrecedenceLowest,
                  i < list.size() - 1 ? "," : std::string());
    if (i < list.size() - 1) {
      sub3.Newline();
    }
  }
  sub3.Print(terminator);
  penalty_multiline_start_next_line += AssessPenalty(sub3.String());

  int penalty = penalty_multiline_start_next_line;
  bool fits_on_current_line = false;
  if (penalty_one_line < penalty_multiline_start_next_line ||
      penalty_multiline_start_same_line < penalty_multiline_start_next_line) {
    fits_on_current_line = true;
    penalty = penalty_one_line;
    if (penalty_multiline_start_same_line < penalty_one_line) {
      penalty = penalty_multiline_start_same_line;
      force_multiline = true;
    }
  } else {
    force_multiline = true;
  }

  if (list.size() == 0 && !force_multiline) {
    // No elements, and not forcing newlines, print nothing.
  } else {
    if (penalty_multiline_start_next_line < penalty_multiline_start_same_line) {
      stack_.push_back(IndentState(margin() + kIndentSize * 2,
                                   continuation_requires_indent, false));
      Newline();
    } else {
      stack_.push_back(
          IndentState(CurrentColumn(), continuation_requires_indent, false));
    }

    for (size_t i = 0; i < list.size(); ++i) {
      const auto& x = list[i];
      if (i > 0) {
        if (fits_on_current_line && !force_multiline)
          Print(" ");
        else
          Newline();
      }
      bool want_comma = i < list.size() - 1 && !x->AsBlockComment();
      Expr(x.get(), kPrecedenceLowest, want_comma ? "," : std::string());
      CHECK(!x->comments() || x->comments()->after().empty());
      if (i < list.size() - 1) {
        if (!want_comma)
          Newline();
      }
    }

    // Trailing comments.
    if (end->comments() && !end->comments()->before().empty()) {
      if (!list.empty())
        Newline();
      for (const auto& c : end->comments()->before()) {
        Newline();
        TrimAndPrintToken(c);
      }
      Newline();
    }
    stack_.pop_back();
  }

  // Defer any end of line comment until we reach the newline.
  if (end->comments() && !end->comments()->suffix().empty()) {
    std::copy(end->comments()->suffix().begin(),
              end->comments()->suffix().end(), std::back_inserter(comments_));
  }

  Print(")");
  Print(suffix);

  if (have_block) {
    Print(" ");
    Sequence(kSequenceStyleBracedBlock, func_call->block()->statements(),
             func_call->block()->End(), false);
  }
  return penalty + (CurrentLine() - start_line) * GetPenaltyForLineBreak();
}

void Printer::InitializeSub(Printer* sub) {
  sub->stack_ = stack_;
  sub->comments_ = comments_;
  sub->penalty_depth_ = penalty_depth_;
  sub->Print(std::string(CurrentColumn(), 'x'));
}

template <class PARSENODE>
bool Printer::ListWillBeMultiline(
    const std::vector<std::unique_ptr<PARSENODE>>& list,
    const ParseNode* end) {
  if (list.size() > 1)
    return true;

  if (end && end->comments() && !end->comments()->before().empty())
    return true;

  // If there's before or suffix line comments, make sure we have a place to put
  // them.
  for (const auto& i : list) {
    if (i->comments() && (!i->comments()->before().empty() ||
                          !i->comments()->suffix().empty())) {
      return true;
    }
  }

  // When a scope is used as a list entry, it's too complicated to go one a
  // single line (the block will always be formatted multiline itself).
  if (list.size() >= 1 && list[0]->AsBlock())
    return true;

  return false;
}

void DoFormat(const ParseNode* root,
              TreeDumpMode dump_tree,
              std::string* output,
              std::string* dump_output) {
  if (dump_tree == TreeDumpMode::kPlainText) {
    std::ostringstream os;
    RenderToText(root->GetJSONNode(), 0, os);
    *dump_output = os.str();
  } else if (dump_tree == TreeDumpMode::kJSON) {
    std::string os;
    base::JSONWriter::WriteWithOptions(
        root->GetJSONNode(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &os);
    *dump_output = os;
  }

  Printer pr;
  pr.Block(root);
  *output = pr.String();
}

}  // namespace

bool FormatJsonToString(const std::string& json, std::string* output) {
  base::JSONReader reader;
  std::unique_ptr<base::Value> json_root = reader.Read(json);
  std::unique_ptr<ParseNode> root = ParseNode::BuildFromJSON(*json_root);
  DoFormat(root.get(), TreeDumpMode::kInactive, output, nullptr);
  return true;
}

bool FormatStringToString(const std::string& input,
                          TreeDumpMode dump_tree,
                          std::string* output,
                          std::string* dump_output) {
  SourceFile source_file;
  InputFile file(source_file);
  file.SetContents(input);
  Err err;
  // Tokenize.
  std::vector<Token> tokens =
      Tokenizer::Tokenize(&file, &err, WhitespaceTransform::kInvalidToSpace);
  if (err.has_error()) {
    err.PrintToStdout();
    return false;
  }

  // Parse.
  std::unique_ptr<ParseNode> parse_node = Parser::Parse(tokens, &err);
  if (err.has_error()) {
    err.PrintToStdout();
    return false;
  }

  DoFormat(parse_node.get(), dump_tree, output, dump_output);
  return true;
}

int RunFormat(const std::vector<std::string>& args) {
#if defined(OS_WIN)
  // Set to binary mode to prevent converting newlines to \r\n.
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);
#endif

  bool dry_run =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kSwitchDryRun);
  TreeDumpMode dump_tree = TreeDumpMode::kInactive;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kSwitchDumpTree)) {
    std::string tree_type =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueString(
            kSwitchDumpTree);
    if (tree_type == kSwitchTreeTypeJSON) {
      dump_tree = TreeDumpMode::kJSON;
    } else if (tree_type.empty() || tree_type == kSwitchTreeTypeText) {
      dump_tree = TreeDumpMode::kPlainText;
    } else {
      Err(Location(), tree_type +
                          " is an invalid value for --dump-tree. Specify "
                          "\"" +
                          kSwitchTreeTypeText + "\" or \"" +
                          kSwitchTreeTypeJSON + "\".\n")
          .PrintToStdout();
      return 1;
    }
  }
  bool from_stdin =
      base::CommandLine::ForCurrentProcess()->HasSwitch(kSwitchStdin);

  if (dry_run) {
    // --dry-run only works with an actual file to compare to.
    from_stdin = false;
  }

  bool quiet =
      base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kQuiet);

  if (from_stdin) {
    if (args.size() != 0) {
      Err(Location(), "Expecting no arguments when reading from stdin.\n")
          .PrintToStdout();
      return 1;
    }
    std::string input = ReadStdin();
    std::string output;
    std::string dump_output;
    if (!FormatStringToString(input, dump_tree, &output, &dump_output))
      return 1;
    printf("%s", dump_output.c_str());
    printf("%s", output.c_str());
    return 0;
  }

  if (args.size() == 0) {
    Err(Location(), "Expecting one or more arguments, see `gn help format`.\n")
        .PrintToStdout();
    return 1;
  }

  Setup setup;
  SourceDir source_dir =
      SourceDirForCurrentDirectory(setup.build_settings().root_path());

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kSwitchReadTree)) {
    std::string tree_type =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueString(
            kSwitchReadTree);
    if (tree_type != kSwitchTreeTypeJSON) {
      Err(Location(), "Only json supported for read-tree.\n").PrintToStdout();
      return 1;
    }

    if (args.size() != 1) {
      Err(Location(),
          "Expect exactly one .gn when reading tree from json on stdin.\n")
          .PrintToStdout();
      return 1;
    }
    Err err;
    SourceFile file =
        source_dir.ResolveRelativeFile(Value(nullptr, args[0]), &err);
    if (err.has_error()) {
      err.PrintToStdout();
      return 1;
    }
    base::FilePath to_format = setup.build_settings().GetFullPath(file);
    std::string output;
    FormatJsonToString(ReadStdin(), &output);
    if (base::WriteFile(to_format, output.data(),
                        static_cast<int>(output.size())) == -1) {
      Err(Location(), std::string("Failed to write output to \"") +
                          FilePathToUTF8(to_format) + std::string("\"."))
          .PrintToStdout();
      return 1;
    }
    if (!quiet) {
      printf("Wrote rebuilt from json to '%s'.\n",
             FilePathToUTF8(to_format).c_str());
    }
    return 0;
  }

  // TODO(scottmg): Eventually, this list of files should be processed in
  // parallel.
  int exit_code = 0;
  for (const auto& arg : args) {
    Err err;
    SourceFile file = source_dir.ResolveRelativeFile(Value(nullptr, arg), &err);
    if (err.has_error()) {
      err.PrintToStdout();
      exit_code = 1;
      continue;
    }

    base::FilePath to_format = setup.build_settings().GetFullPath(file);
    std::string original_contents;
    if (!base::ReadFileToString(to_format, &original_contents)) {
      Err(Location(),
          std::string("Couldn't read \"") + FilePathToUTF8(to_format))
          .PrintToStdout();
      exit_code = 1;
      continue;
    }

    std::string output_string;
    std::string dump_output_string;
    if (!FormatStringToString(original_contents, dump_tree, &output_string,
                              &dump_output_string)) {
      exit_code = 1;
      continue;
    }
    printf("%s", dump_output_string.c_str());
    if (dump_tree == TreeDumpMode::kInactive) {
      if (dry_run) {
        if (original_contents != output_string) {
          printf("%s\n", arg.c_str());
          exit_code = 2;
        }
        continue;
      }
      // Update the file in-place.
      if (original_contents != output_string) {
        if (base::WriteFile(to_format, output_string.data(),
                            static_cast<int>(output_string.size())) == -1) {
          Err(Location(),
              std::string("Failed to write formatted output back to \"") +
                  FilePathToUTF8(to_format) + std::string("\"."))
              .PrintToStdout();
          exit_code = 1;
          continue;
        }
        if (!quiet) {
          printf("Wrote formatted to '%s'.\n",
                 FilePathToUTF8(to_format).c_str());
        }
      }
    }
  }

  return exit_code;
}

}  // namespace commands
