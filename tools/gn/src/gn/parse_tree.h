// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_PARSE_TREE_H_
#define TOOLS_GN_PARSE_TREE_H_

#include <stddef.h>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "base/values.h"
#include "gn/err.h"
#include "gn/token.h"
#include "gn/value.h"

class AccessorNode;
class BinaryOpNode;
class BlockCommentNode;
class BlockNode;
class ConditionNode;
class EndNode;
class FunctionCallNode;
class IdentifierNode;
class ListNode;
class LiteralNode;
class Scope;
class UnaryOpNode;

// Dictionary keys used for JSON-formatted tree dump.
extern const char kJsonNodeChild[];
extern const char kJsonNodeType[];
extern const char kJsonNodeValue[];
extern const char kJsonBeforeComment[];
extern const char kJsonSuffixComment[];
extern const char kJsonAfterComment[];

class Comments {
 public:
  Comments();
  virtual ~Comments();

  const std::vector<Token>& before() const { return before_; }
  void append_before(Token c) { before_.push_back(c); }
  void clear_before() { before_.clear(); }

  const std::vector<Token>& suffix() const { return suffix_; }
  void append_suffix(Token c) { suffix_.push_back(c); }
  // Reverse the order of the suffix comments. When walking the tree in
  // post-order we append suffix comments in reverse order, so this fixes them
  // up.
  void ReverseSuffix();

  const std::vector<Token>& after() const { return after_; }
  void append_after(Token c) { after_.push_back(c); }

 private:
  // Whole line comments before the expression.
  std::vector<Token> before_;

  // End-of-line comments after this expression.
  std::vector<Token> suffix_;

  // For top-level expressions only, after_ lists whole-line comments
  // following the expression.
  std::vector<Token> after_;

  Comments(const Comments&) = delete;
  Comments& operator=(const Comments&) = delete;
};

// ParseNode -------------------------------------------------------------------

// A node in the AST.
class ParseNode {
 public:
  ParseNode();
  virtual ~ParseNode();

  virtual const AccessorNode* AsAccessor() const;
  virtual const BinaryOpNode* AsBinaryOp() const;
  virtual const BlockCommentNode* AsBlockComment() const;
  virtual const BlockNode* AsBlock() const;
  virtual const ConditionNode* AsCondition() const;
  virtual const EndNode* AsEnd() const;
  virtual const FunctionCallNode* AsFunctionCall() const;
  virtual const IdentifierNode* AsIdentifier() const;
  virtual const ListNode* AsList() const;
  virtual const LiteralNode* AsLiteral() const;
  virtual const UnaryOpNode* AsUnaryOp() const;

  virtual Value Execute(Scope* scope, Err* err) const = 0;

  virtual LocationRange GetRange() const = 0;

  // Returns an error with the given messages and the range set to something
  // that indicates this node.
  virtual Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const = 0;

  // Generates a representation of this node in base::Value, to be used for
  // exporting the tree as a JSON or formatted text with indents.
  virtual base::Value GetJSONNode() const = 0;

  const Comments* comments() const { return comments_.get(); }
  Comments* comments_mutable();

  static std::unique_ptr<ParseNode> BuildFromJSON(const base::Value& value);

 protected:
  // Helper functions for GetJSONNode. Creates and fills a Value object with
  // given type (and value).
  base::Value CreateJSONNode(const char* type, LocationRange location) const;
  base::Value CreateJSONNode(const char* type,
                             std::string_view value,
                             LocationRange location) const;

 private:
  // Helper function for CreateJSONNode.
  void AddCommentsJSONNodes(base::Value* out_value) const;

  std::unique_ptr<Comments> comments_;

  ParseNode(const ParseNode&) = delete;
  ParseNode& operator=(const ParseNode&) = delete;
};

// AccessorNode ----------------------------------------------------------------

// Access an array or scope element.
//
// Currently, such values are only read-only. In that you can do:
//   a = obj1.a
//   b = obj2[0]
// But not
//   obj1.a = 5
//   obj2[0] = 6
//
// In the current design where the dot operator is used only for templates, we
// explicitly don't want to allow you to do "invoker.foo = 5", so if we added
// support for accessors to be lvalues, we would also need to add some concept
// of a constant scope. Supporting this would also add a lot of complications
// to the operator= implementation, since some accessors might return values
// in the const root scope that shouldn't be modified. Without a strong
// use-case for this, it seems simpler to just disallow it.
//
// Additionally, the left-hand-side of the accessor must currently be an
// identifier. So you can't do things like:
//   function_call()[1]
//   a = b.c.d
// These are easier to implement if we needed them but given the very limited
// use cases for this, it hasn't seemed worth the bother.
class AccessorNode : public ParseNode {
 public:
  AccessorNode();
  ~AccessorNode() override;

  const AccessorNode* AsAccessor() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<AccessorNode> NewFromJSON(const base::Value& value);

  // Base is the thing on the left of the [] or dot, currently always required
  // to be an identifier token.
  const Token& base() const { return base_; }
  void set_base(const Token& b) { base_ = b; }

  // Subscript is the expression inside the []. Will be null if member is set.
  const ParseNode* subscript() const { return subscript_.get(); }
  void set_subscript(std::unique_ptr<ParseNode> key) {
    subscript_ = std::move(key);
  }

  // The member is the identifier on the right hand side of the dot. Will be
  // null if the index is set.
  const IdentifierNode* member() const { return member_.get(); }
  void set_member(std::unique_ptr<IdentifierNode> i) { member_ = std::move(i); }

  void SetNewLocation(int line_number);

  // Evaluates the index for list accessor operations and range checks it
  // against the max length of the list. If the index is OK, sets
  // |*computed_index| and returns true. Otherwise sets the |*err| and returns
  // false.
  bool ComputeAndValidateListIndex(Scope* scope,
                                   size_t max_len,
                                   size_t* computed_index,
                                   Err* err) const;

  static constexpr const char* kDumpNodeName = "ACCESSOR";

 private:
  Value ExecuteSubscriptAccess(Scope* scope, Err* err) const;
  Value ExecuteArrayAccess(Scope* scope,
                           const Value* base_value,
                           Err* err) const;
  Value ExecuteScopeSubscriptAccess(Scope* scope,
                                    const Value* base_value,
                                    Err* err) const;
  Value ExecuteScopeAccess(Scope* scope, Err* err) const;

  static constexpr const char* kDumpAccessorKind = "accessor_kind";
  static constexpr const char* kDumpAccessorKindSubscript = "subscript";
  static constexpr const char* kDumpAccessorKindMember = "member";

  Token base_;

  // Either index or member will be set according to what type of access this
  // is.
  std::unique_ptr<ParseNode> subscript_;
  std::unique_ptr<IdentifierNode> member_;

  AccessorNode(const AccessorNode&) = delete;
  AccessorNode& operator=(const AccessorNode&) = delete;
};

// BinaryOpNode ----------------------------------------------------------------

class BinaryOpNode : public ParseNode {
 public:
  BinaryOpNode();
  ~BinaryOpNode() override;

  const BinaryOpNode* AsBinaryOp() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<BinaryOpNode> NewFromJSON(const base::Value& value);

  const Token& op() const { return op_; }
  void set_op(const Token& t) { op_ = t; }

  const ParseNode* left() const { return left_.get(); }
  void set_left(std::unique_ptr<ParseNode> left) { left_ = std::move(left); }

  const ParseNode* right() const { return right_.get(); }
  void set_right(std::unique_ptr<ParseNode> right) {
    right_ = std::move(right);
  }

  static constexpr const char* kDumpNodeName = "BINARY";

 private:
  std::unique_ptr<ParseNode> left_;
  Token op_;
  std::unique_ptr<ParseNode> right_;

  BinaryOpNode(const BinaryOpNode&) = delete;
  BinaryOpNode& operator=(const BinaryOpNode&) = delete;
};

// BlockNode -------------------------------------------------------------------

class BlockNode : public ParseNode {
 public:
  // How Execute manages the scopes and results.
  enum ResultMode {
    // Creates a new scope for the execution of this block and returns it as
    // a Value from Execute().
    RETURNS_SCOPE,

    // Executes in the context of the calling scope (variables set will go
    // into the invoking scope) and Execute will return an empty Value.
    DISCARDS_RESULT
  };

  BlockNode(ResultMode result_mode);
  ~BlockNode() override;

  const BlockNode* AsBlock() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<BlockNode> NewFromJSON(const base::Value& value);

  void set_begin_token(const Token& t) { begin_token_ = t; }
  void set_end(std::unique_ptr<EndNode> e) { end_ = std::move(e); }
  const EndNode* End() const { return end_.get(); }

  ResultMode result_mode() const { return result_mode_; }

  const std::vector<std::unique_ptr<ParseNode>>& statements() const {
    return statements_;
  }
  void append_statement(std::unique_ptr<ParseNode> s) {
    statements_.push_back(std::move(s));
  }

  static constexpr const char* kDumpNodeName = "BLOCK";

 private:
  static constexpr const char* kDumpResultMode = "result_mode";
  static constexpr const char* kDumpResultModeReturnsScope = "returns_scope";
  static constexpr const char* kDumpResultModeDiscardsResult =
      "discards_result";

  const ResultMode result_mode_;

  // Tokens corresponding to { and }, if any (may be NULL). The end is stored
  // in a custom parse node so that it can have comments hung off of it.
  Token begin_token_;
  std::unique_ptr<EndNode> end_;

  std::vector<std::unique_ptr<ParseNode>> statements_;

  BlockNode(const BlockNode&) = delete;
  BlockNode& operator=(const BlockNode&) = delete;
};

// ConditionNode ---------------------------------------------------------------

class ConditionNode : public ParseNode {
 public:
  ConditionNode();
  ~ConditionNode() override;

  const ConditionNode* AsCondition() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<ConditionNode> NewFromJSON(const base::Value& value);

  void set_if_token(const Token& token) { if_token_ = token; }

  const ParseNode* condition() const { return condition_.get(); }
  void set_condition(std::unique_ptr<ParseNode> c) {
    condition_ = std::move(c);
  }

  const BlockNode* if_true() const { return if_true_.get(); }
  void set_if_true(std::unique_ptr<BlockNode> t) { if_true_ = std::move(t); }

  // This is either empty, a block (for the else clause), or another
  // condition.
  const ParseNode* if_false() const { return if_false_.get(); }
  void set_if_false(std::unique_ptr<ParseNode> f) { if_false_ = std::move(f); }

  static constexpr const char* kDumpNodeName = "CONDITION";

 private:
  // Token corresponding to the "if" string.
  Token if_token_;

  std::unique_ptr<ParseNode> condition_;  // Always non-null.
  std::unique_ptr<BlockNode> if_true_;    // Always non-null.
  std::unique_ptr<ParseNode> if_false_;   // May be null.

  ConditionNode(const ConditionNode&) = delete;
  ConditionNode& operator=(const ConditionNode&) = delete;
};

// FunctionCallNode ------------------------------------------------------------

class FunctionCallNode : public ParseNode {
 public:
  FunctionCallNode();
  ~FunctionCallNode() override;

  const FunctionCallNode* AsFunctionCall() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<FunctionCallNode> NewFromJSON(
      const base::Value& value);

  const Token& function() const { return function_; }
  void set_function(Token t) { function_ = t; }

  const ListNode* args() const { return args_.get(); }
  void set_args(std::unique_ptr<ListNode> a) { args_ = std::move(a); }

  const BlockNode* block() const { return block_.get(); }
  void set_block(std::unique_ptr<BlockNode> b) { block_ = std::move(b); }

  void SetNewLocation(int line_number);

  static constexpr const char* kDumpNodeName = "FUNCTION";

 private:
  Token function_;
  std::unique_ptr<ListNode> args_;
  std::unique_ptr<BlockNode> block_;  // May be null.

  FunctionCallNode(const FunctionCallNode&) = delete;
  FunctionCallNode& operator=(const FunctionCallNode&) = delete;
};

// IdentifierNode --------------------------------------------------------------

class IdentifierNode : public ParseNode {
 public:
  IdentifierNode();
  explicit IdentifierNode(const Token& token);
  ~IdentifierNode() override;

  const IdentifierNode* AsIdentifier() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<IdentifierNode> NewFromJSON(const base::Value& value);

  const Token& value() const { return value_; }
  void set_value(const Token& t) { value_ = t; }

  void SetNewLocation(int line_number);

  static constexpr const char* kDumpNodeName = "IDENTIFIER";

 private:
  Token value_;

  IdentifierNode(const IdentifierNode&) = delete;
  IdentifierNode& operator=(const IdentifierNode&) = delete;
};

// ListNode --------------------------------------------------------------------

class ListNode : public ParseNode {
 public:
  ListNode();
  ~ListNode() override;

  const ListNode* AsList() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<ListNode> NewFromJSON(const base::Value& value);

  void set_begin_token(const Token& t) { begin_token_ = t; }
  const Token& Begin() const { return begin_token_; }
  void set_end(std::unique_ptr<EndNode> e) { end_ = std::move(e); }
  const EndNode* End() const { return end_.get(); }

  void append_item(std::unique_ptr<ParseNode> s) {
    contents_.push_back(std::move(s));
  }
  const std::vector<std::unique_ptr<const ParseNode>>& contents() const {
    return contents_;
  }

  void SortAsStringsList();
  void SortAsTargetsList();

  struct SortRange {
    size_t begin;
    size_t end;
    SortRange(size_t begin, size_t end) : begin(begin), end(end) {}
  };
  // Only public for testing.
  std::vector<SortRange> GetSortRanges() const;

  static constexpr const char* kDumpNodeName = "LIST";

 private:
  template <typename Comparator>
  void SortList(Comparator comparator);

  // Tokens corresponding to the [ and ]. The end token is stored in inside an
  // custom parse node so that it can have comments hung off of it.
  Token begin_token_;
  std::unique_ptr<EndNode> end_;

  std::vector<std::unique_ptr<const ParseNode>> contents_;

  ListNode(const ListNode&) = delete;
  ListNode& operator=(const ListNode&) = delete;
};

// LiteralNode -----------------------------------------------------------------

class LiteralNode : public ParseNode {
 public:
  LiteralNode();
  explicit LiteralNode(const Token& token);
  ~LiteralNode() override;

  const LiteralNode* AsLiteral() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<LiteralNode> NewFromJSON(const base::Value& value);

  const Token& value() const { return value_; }
  void set_value(const Token& t) { value_ = t; }

  void SetNewLocation(int line_number);

  static constexpr const char* kDumpNodeName = "LITERAL";

 private:
  Token value_;

  LiteralNode(const LiteralNode&) = delete;
  LiteralNode& operator=(const LiteralNode&) = delete;
};

// UnaryOpNode -----------------------------------------------------------------

class UnaryOpNode : public ParseNode {
 public:
  UnaryOpNode();
  ~UnaryOpNode() override;

  const UnaryOpNode* AsUnaryOp() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<UnaryOpNode> NewFromJSON(const base::Value& value);

  const Token& op() const { return op_; }
  void set_op(const Token& t) { op_ = t; }

  const ParseNode* operand() const { return operand_.get(); }
  void set_operand(std::unique_ptr<ParseNode> operand) {
    operand_ = std::move(operand);
  }

  static constexpr const char* kDumpNodeName = "UNARY";

 private:
  Token op_;
  std::unique_ptr<ParseNode> operand_;

  UnaryOpNode(const UnaryOpNode&) = delete;
  UnaryOpNode& operator=(const UnaryOpNode&) = delete;
};

// BlockCommentNode ------------------------------------------------------------

// This node type is only used for standalone comments (that is, those not
// specifically attached to another syntax element. The most common of these
// is a standard header block. This node contains only the last line of such
// a comment block as the anchor, and other lines of the block comment are
// hung off of it as Before comments, similar to other syntax elements.
class BlockCommentNode : public ParseNode {
 public:
  BlockCommentNode();
  ~BlockCommentNode() override;

  const BlockCommentNode* AsBlockComment() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<BlockCommentNode> NewFromJSON(
      const base::Value& value);

  const Token& comment() const { return comment_; }
  void set_comment(const Token& t) { comment_ = t; }

  static constexpr const char* kDumpNodeName = "BLOCK_COMMENT";

 private:
  Token comment_;

  BlockCommentNode(const BlockCommentNode&) = delete;
  BlockCommentNode& operator=(const BlockCommentNode&) = delete;
};

// EndNode ---------------------------------------------------------------------

// This node type is used as the end_ object for lists and blocks (rather than
// just the end ']', '}', or ')' token). This is so that during formatting
// traversal there is a node that appears at the end of the block to which
// comments can be attached.
class EndNode : public ParseNode {
 public:
  explicit EndNode(const Token& token);
  ~EndNode() override;

  const EndNode* AsEnd() const override;
  Value Execute(Scope* scope, Err* err) const override;
  LocationRange GetRange() const override;
  Err MakeErrorDescribing(
      const std::string& msg,
      const std::string& help = std::string()) const override;
  base::Value GetJSONNode() const override;
  static std::unique_ptr<EndNode> NewFromJSON(const base::Value& value);

  const Token& value() const { return value_; }
  void set_value(const Token& t) { value_ = t; }

  static constexpr const char* kDumpNodeName = "END";

 private:
  Token value_;

  EndNode(const EndNode&) = delete;
  EndNode& operator=(const EndNode&) = delete;
};

#endif  // TOOLS_GN_PARSE_TREE_H_
