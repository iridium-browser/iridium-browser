// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/parse_tree.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <tuple>

#include "base/json/string_escape.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "gn/functions.h"
#include "gn/operators.h"
#include "gn/scope.h"
#include "gn/string_utils.h"

// Dictionary keys used for JSON-formatted tree dump.
const char kJsonNodeChild[] = "child";
const char kJsonNodeType[] = "type";
const char kJsonNodeValue[] = "value";
const char kJsonBeforeComment[] = "before_comment";
const char kJsonSuffixComment[] = "suffix_comment";
const char kJsonAfterComment[] = "after_comment";
const char kJsonLocation[] = "location";
const char kJsonLocationBeginLine[] = "begin_line";
const char kJsonLocationBeginColumn[] = "begin_column";
const char kJsonLocationEndLine[] = "end_line";
const char kJsonLocationEndColumn[] = "end_column";

// Used by Block and List.
const char kJsonBeginToken[] = "begin_token";
const char kJsonEnd[] = "end";

namespace {

enum DepsCategory {
  DEPS_CATEGORY_LOCAL,
  DEPS_CATEGORY_RELATIVE,
  DEPS_CATEGORY_ABSOLUTE,
  DEPS_CATEGORY_OTHER,
};

DepsCategory GetDepsCategory(std::string_view deps) {
  if (deps.length() < 2 || deps[0] != '"' || deps[deps.size() - 1] != '"')
    return DEPS_CATEGORY_OTHER;

  if (deps[1] == ':')
    return DEPS_CATEGORY_LOCAL;

  if (deps[1] == '/')
    return DEPS_CATEGORY_ABSOLUTE;

  return DEPS_CATEGORY_RELATIVE;
}

std::tuple<std::string_view, std::string_view> SplitAtFirst(
    std::string_view str,
    char c) {
  if (!base::StartsWith(str, "\"", base::CompareCase::SENSITIVE) ||
      !base::EndsWith(str, "\"", base::CompareCase::SENSITIVE))
    return std::make_tuple(str, std::string_view());

  str = str.substr(1, str.length() - 2);
  size_t index_of_first = str.find(c);
  return std::make_tuple(str.substr(0, index_of_first),
                         index_of_first != std::string_view::npos
                             ? str.substr(index_of_first + 1)
                             : std::string_view());
}

bool IsSortRangeSeparator(const ParseNode* node, const ParseNode* prev) {
  // If it's a block comment, or has an attached comment with a blank line
  // before it, then we break the range at this point.
  return node->AsBlockComment() != nullptr ||
         (prev && node->comments() && !node->comments()->before().empty() &&
          (node->GetRange().begin().line_number() >
           prev->GetRange().end().line_number() +
               static_cast<int>(node->comments()->before().size() + 1)));
}

std::string_view GetStringRepresentation(const ParseNode* node) {
  DCHECK(node->AsLiteral() || node->AsIdentifier() || node->AsAccessor());
  if (node->AsLiteral())
    return node->AsLiteral()->value().value();
  else if (node->AsIdentifier())
    return node->AsIdentifier()->value().value();
  else if (node->AsAccessor())
    return node->AsAccessor()->base().value();
  return std::string_view();
}

void AddLocationJSONNodes(base::Value* dict, LocationRange location) {
  base::Value loc(base::Value::Type::DICTIONARY);
  loc.SetKey(kJsonLocationBeginLine,
             base::Value(location.begin().line_number()));
  loc.SetKey(kJsonLocationBeginColumn,
             base::Value(location.begin().column_number()));
  loc.SetKey(kJsonLocationEndLine, base::Value(location.end().line_number()));
  loc.SetKey(kJsonLocationEndColumn,
             base::Value(location.end().column_number()));
  dict->SetKey(kJsonLocation, std::move(loc));
}

Location GetBeginLocationFromJSON(const base::Value& value) {
  int line =
      value.FindKey(kJsonLocation)->FindKey(kJsonLocationBeginLine)->GetInt();
  int column =
      value.FindKey(kJsonLocation)->FindKey(kJsonLocationBeginColumn)->GetInt();
  return Location(nullptr, line, column);
}

void GetCommentsFromJSON(ParseNode* node, const base::Value& value) {
  Comments* comments = node->comments_mutable();

  Location loc = GetBeginLocationFromJSON(value);

  auto loc_for = [&loc](int line) {
    return Location(nullptr, loc.line_number() + line, loc.column_number());
  };

  if (value.FindKey(kJsonBeforeComment)) {
    int line = 0;
    for (const auto& c : value.FindKey(kJsonBeforeComment)->GetList()) {
      comments->append_before(
          Token::ClassifyAndMake(loc_for(line), c.GetString()));
      ++line;
    }
  }

  if (value.FindKey(kJsonSuffixComment)) {
    int line = 0;
    for (const auto& c : value.FindKey(kJsonSuffixComment)->GetList()) {
      comments->append_suffix(
          Token::ClassifyAndMake(loc_for(line), c.GetString()));
      ++line;
    }
  }

  if (value.FindKey(kJsonAfterComment)) {
    int line = 0;
    for (const auto& c : value.FindKey(kJsonAfterComment)->GetList()) {
      comments->append_after(
          Token::ClassifyAndMake(loc_for(line), c.GetString()));
      ++line;
    }
  }
}

Token TokenFromValue(const base::Value& value) {
  return Token::ClassifyAndMake(GetBeginLocationFromJSON(value),
                                value.FindKey(kJsonNodeValue)->GetString());
}

}  // namespace

Comments::Comments() = default;

Comments::~Comments() = default;

void Comments::ReverseSuffix() {
  for (int i = 0, j = static_cast<int>(suffix_.size() - 1); i < j; ++i, --j)
    std::swap(suffix_[i], suffix_[j]);
}

ParseNode::ParseNode() = default;

ParseNode::~ParseNode() = default;

const AccessorNode* ParseNode::AsAccessor() const {
  return nullptr;
}
const BinaryOpNode* ParseNode::AsBinaryOp() const {
  return nullptr;
}
const BlockCommentNode* ParseNode::AsBlockComment() const {
  return nullptr;
}
const BlockNode* ParseNode::AsBlock() const {
  return nullptr;
}
const ConditionNode* ParseNode::AsCondition() const {
  return nullptr;
}
const EndNode* ParseNode::AsEnd() const {
  return nullptr;
}
const FunctionCallNode* ParseNode::AsFunctionCall() const {
  return nullptr;
}
const IdentifierNode* ParseNode::AsIdentifier() const {
  return nullptr;
}
const ListNode* ParseNode::AsList() const {
  return nullptr;
}
const LiteralNode* ParseNode::AsLiteral() const {
  return nullptr;
}
const UnaryOpNode* ParseNode::AsUnaryOp() const {
  return nullptr;
}

Comments* ParseNode::comments_mutable() {
  if (!comments_)
    comments_ = std::make_unique<Comments>();
  return comments_.get();
}

base::Value ParseNode::CreateJSONNode(const char* type,
                                      LocationRange location) const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey(kJsonNodeType, base::Value(type));
  AddLocationJSONNodes(&dict, location);
  AddCommentsJSONNodes(&dict);
  return dict;
}

base::Value ParseNode::CreateJSONNode(const char* type,
                                      std::string_view value,
                                      LocationRange location) const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetKey(kJsonNodeType, base::Value(type));
  dict.SetKey(kJsonNodeValue, base::Value(value));
  AddLocationJSONNodes(&dict, location);
  AddCommentsJSONNodes(&dict);
  return dict;
}

void ParseNode::AddCommentsJSONNodes(base::Value* out_value) const {
  if (comments_) {
    if (comments_->before().size()) {
      base::Value comment_values(base::Value::Type::LIST);
      for (const auto& token : comments_->before())
        comment_values.GetList().push_back(base::Value(token.value()));
      out_value->SetKey(kJsonBeforeComment, std::move(comment_values));
    }
    if (comments_->suffix().size()) {
      base::Value comment_values(base::Value::Type::LIST);
      for (const auto& token : comments_->suffix())
        comment_values.GetList().push_back(base::Value(token.value()));
      out_value->SetKey(kJsonSuffixComment, std::move(comment_values));
    }
    if (comments_->after().size()) {
      base::Value comment_values(base::Value::Type::LIST);
      for (const auto& token : comments_->after())
        comment_values.GetList().push_back(base::Value(token.value()));
      out_value->SetKey(kJsonAfterComment, std::move(comment_values));
    }
  }
}

// static
std::unique_ptr<ParseNode> ParseNode::BuildFromJSON(const base::Value& value) {
  const std::string& str_type = value.FindKey(kJsonNodeType)->GetString();

#define RETURN_IF_MATCHES_NAME(t)               \
  do {                                          \
    if (str_type == t::kDumpNodeName) {         \
      return t::NewFromJSON(value);             \
    }                                           \
  } while(0)

  RETURN_IF_MATCHES_NAME(AccessorNode);
  RETURN_IF_MATCHES_NAME(BinaryOpNode);
  RETURN_IF_MATCHES_NAME(BlockCommentNode);
  RETURN_IF_MATCHES_NAME(BlockNode);
  RETURN_IF_MATCHES_NAME(ConditionNode);
  RETURN_IF_MATCHES_NAME(EndNode);
  RETURN_IF_MATCHES_NAME(FunctionCallNode);
  RETURN_IF_MATCHES_NAME(IdentifierNode);
  RETURN_IF_MATCHES_NAME(ListNode);
  RETURN_IF_MATCHES_NAME(LiteralNode);
  RETURN_IF_MATCHES_NAME(UnaryOpNode);

#undef RETURN_IF_MATCHES_NAME

  NOTREACHED() << str_type;
  return std::unique_ptr<ParseNode>();
}

// AccessorNode ---------------------------------------------------------------

AccessorNode::AccessorNode() = default;

AccessorNode::~AccessorNode() = default;

const AccessorNode* AccessorNode::AsAccessor() const {
  return this;
}

Value AccessorNode::Execute(Scope* scope, Err* err) const {
  if (subscript_)
    return ExecuteSubscriptAccess(scope, err);
  else if (member_)
    return ExecuteScopeAccess(scope, err);
  NOTREACHED();
  return Value();
}

LocationRange AccessorNode::GetRange() const {
  if (subscript_)
    return LocationRange(base_.location(), subscript_->GetRange().end());
  else if (member_)
    return LocationRange(base_.location(), member_->GetRange().end());
  NOTREACHED();
  return LocationRange();
}

Err AccessorNode::MakeErrorDescribing(const std::string& msg,
                                      const std::string& help) const {
  return Err(GetRange(), msg, help);
}

base::Value AccessorNode::GetJSONNode() const {
  base::Value dict(CreateJSONNode(kDumpNodeName, base_.value(), GetRange()));
  base::Value child(base::Value::Type::LIST);
  if (subscript_) {
    child.GetList().push_back(subscript_->GetJSONNode());
    dict.SetKey(kDumpAccessorKind, base::Value(kDumpAccessorKindSubscript));
  } else if (member_) {
    child.GetList().push_back(member_->GetJSONNode());
    dict.SetKey(kDumpAccessorKind, base::Value(kDumpAccessorKindMember));
  }
  dict.SetKey(kJsonNodeChild, std::move(child));
  return dict;
}

#define DECLARE_CHILD_AS_LIST_OR_FAIL()                     \
  const base::Value* child = value.FindKey(kJsonNodeChild); \
  if (!child || !child->is_list()) {                        \
    return nullptr;                                         \
  }                                                         \
  (void)(0) // this is to supress extra semicolon warning.

// static
std::unique_ptr<AccessorNode> AccessorNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<AccessorNode>();
  DECLARE_CHILD_AS_LIST_OR_FAIL();
  ret->base_ = TokenFromValue(value);
  const base::Value::ListStorage& children = child->GetList();
  const std::string& kind = value.FindKey(kDumpAccessorKind)->GetString();
  if (kind == kDumpAccessorKindSubscript) {
    ret->subscript_ = ParseNode::BuildFromJSON(children[0]);
  } else if (kind == kDumpAccessorKindMember) {
    ret->member_ = IdentifierNode::NewFromJSON(children[0]);
  }
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

Value AccessorNode::ExecuteSubscriptAccess(Scope* scope, Err* err) const {
  const Value* base_value = scope->GetValue(base_.value(), true);
  if (!base_value) {
    *err = MakeErrorDescribing("Undefined identifier.");
    return Value();
  }
  if (base_value->type() == Value::LIST) {
    return ExecuteArrayAccess(scope, base_value, err);
  } else if (base_value->type() == Value::SCOPE) {
    return ExecuteScopeSubscriptAccess(scope, base_value, err);
  } else {
    *err = MakeErrorDescribing(
        std::string("Expecting either a list or a scope for subscript, got ") +
        Value::DescribeType(base_value->type()) + ".");
    return Value();
  }
}

Value AccessorNode::ExecuteArrayAccess(Scope* scope,
                                       const Value* base_value,
                                       Err* err) const {
  size_t index = 0;
  if (!ComputeAndValidateListIndex(scope, base_value->list_value().size(),
                                   &index, err))
    return Value();
  return base_value->list_value()[index];
}

Value AccessorNode::ExecuteScopeSubscriptAccess(Scope* scope,
                                                const Value* base_value,
                                                Err* err) const {
  Value key_value = subscript_->Execute(scope, err);
  if (err->has_error())
    return Value();
  if (!key_value.VerifyTypeIs(Value::STRING, err))
    return Value();
  const Value* result =
      base_value->scope_value()->GetValue(key_value.string_value());
  if (!result) {
    *err =
        Err(subscript_.get(), "No value named \"" + key_value.string_value() +
                                  "\" in scope \"" + base_.value() + "\"");
    return Value();
  }
  return *result;
}

Value AccessorNode::ExecuteScopeAccess(Scope* scope, Err* err) const {
  // We jump through some hoops here since ideally a.b will count "b" as
  // accessed in the given scope. The value "a" might be in some normal nested
  // scope and we can modify it, but it might also be inherited from the
  // readonly root scope and we can't do used variable tracking on it. (It's
  // not legal to const cast it away since the root scope will be in readonly
  // mode and being accessed from multiple threads without locking.) So this
  // code handles both cases.
  const Value* result = nullptr;

  // Look up the value in the scope named by "base_".
  Value* mutable_base_value =
      scope->GetMutableValue(base_.value(), Scope::SEARCH_NESTED, true);
  if (mutable_base_value) {
    // Common case: base value is mutable so we can track variable accesses
    // for unused value warnings.
    if (!mutable_base_value->VerifyTypeIs(Value::SCOPE, err))
      return Value();
    result = mutable_base_value->scope_value()->GetValue(
        member_->value().value(), true);
  } else {
    // Fall back to see if the value is on a read-only scope.
    const Value* const_base_value = scope->GetValue(base_.value(), true);
    if (const_base_value) {
      // Read only value, don't try to mark the value access as a "used" one.
      if (!const_base_value->VerifyTypeIs(Value::SCOPE, err))
        return Value();
      result =
          const_base_value->scope_value()->GetValue(member_->value().value());
    } else {
      *err = Err(base_, "Undefined identifier.");
      return Value();
    }
  }

  if (!result) {
    *err = Err(member_.get(), "No value named \"" + member_->value().value() +
                                  "\" in scope \"" + base_.value() + "\"");
    return Value();
  }
  return *result;
}

void AccessorNode::SetNewLocation(int line_number) {
  Location old = base_.location();
  base_.set_location(Location(old.file(), line_number, old.column_number()));
}

bool AccessorNode::ComputeAndValidateListIndex(Scope* scope,
                                               size_t max_len,
                                               size_t* computed_index,
                                               Err* err) const {
  Value index_value = subscript_->Execute(scope, err);
  if (err->has_error())
    return false;
  if (!index_value.VerifyTypeIs(Value::INTEGER, err))
    return false;

  int64_t index_int = index_value.int_value();
  if (index_int < 0) {
    *err = Err(subscript_->GetRange(), "Negative array subscript.",
               "You gave me " + base::Int64ToString(index_int) + ".");
    return false;
  }
  if (max_len == 0) {
    *err = Err(subscript_->GetRange(), "Array subscript out of range.",
               "You gave me " + base::Int64ToString(index_int) + " but the " +
                   "array has no elements.");
    return false;
  }
  size_t index_sizet = static_cast<size_t>(index_int);
  if (index_sizet >= max_len) {
    *err = Err(subscript_->GetRange(), "Array subscript out of range.",
               "You gave me " + base::Int64ToString(index_int) +
                   " but I was expecting something from 0 to " +
                   base::NumberToString(max_len - 1) + ", inclusive.");
    return false;
  }

  *computed_index = index_sizet;
  return true;
}

// BinaryOpNode ---------------------------------------------------------------

BinaryOpNode::BinaryOpNode() = default;

BinaryOpNode::~BinaryOpNode() = default;

const BinaryOpNode* BinaryOpNode::AsBinaryOp() const {
  return this;
}

Value BinaryOpNode::Execute(Scope* scope, Err* err) const {
  return ExecuteBinaryOperator(scope, this, left_.get(), right_.get(), err);
}

LocationRange BinaryOpNode::GetRange() const {
  return left_->GetRange().Union(right_->GetRange());
}

Err BinaryOpNode::MakeErrorDescribing(const std::string& msg,
                                      const std::string& help) const {
  return Err(op_, msg, help);
}

base::Value BinaryOpNode::GetJSONNode() const {
  base::Value dict(CreateJSONNode(kDumpNodeName, op_.value(), GetRange()));
  base::Value child(base::Value::Type::LIST);
  child.GetList().push_back(left_->GetJSONNode());
  child.GetList().push_back(right_->GetJSONNode());
  dict.SetKey(kJsonNodeChild, std::move(child));
  return dict;
}

// static
std::unique_ptr<BinaryOpNode> BinaryOpNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<BinaryOpNode>();
  DECLARE_CHILD_AS_LIST_OR_FAIL();
  const base::Value::ListStorage& children = child->GetList();
  ret->left_ = ParseNode::BuildFromJSON(children[0]);
  ret->right_ = ParseNode::BuildFromJSON(children[1]);
  ret->op_ = TokenFromValue(value);
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

// BlockNode ------------------------------------------------------------------

BlockNode::BlockNode(ResultMode result_mode) : result_mode_(result_mode) {}

BlockNode::~BlockNode() = default;

const BlockNode* BlockNode::AsBlock() const {
  return this;
}

Value BlockNode::Execute(Scope* enclosing_scope, Err* err) const {
  std::unique_ptr<Scope> nested_scope;  // May be null.

  Scope* execution_scope;  // Either the enclosing_scope or nested_scope.
  if (result_mode_ == RETURNS_SCOPE) {
    // Create a nested scope to save the values for returning.
    nested_scope = std::make_unique<Scope>(enclosing_scope);
    execution_scope = nested_scope.get();
  } else {
    // Use the enclosing scope. Modifications will go into this also (for
    // example, if conditions and loops).
    execution_scope = enclosing_scope;
  }

  for (size_t i = 0; i < statements_.size() && !err->has_error(); i++) {
    // Check for trying to execute things with no side effects in a block.
    //
    // A BlockNode here means that somebody has a free-floating { }.
    // Technically this can have side effects since it could generated targets,
    // but we don't want to allow this since it creates ambiguity when
    // immediately following a function call that takes no block. By not
    // allowing free-floating blocks that aren't passed anywhere or assigned to
    // anything, this ambiguity is resolved.
    const ParseNode* cur = statements_[i].get();
    if (cur->AsList() || cur->AsLiteral() || cur->AsUnaryOp() ||
        cur->AsIdentifier() || cur->AsBlock()) {
      *err = cur->MakeErrorDescribing(
          "This statement has no effect.",
          "Either delete it or do something with the result.");
      return Value();
    }
    cur->Execute(execution_scope, err);
  }

  if (result_mode_ == RETURNS_SCOPE) {
    // Clear the reference to the containing scope. This will be passed in
    // a value whose lifetime will not be related to the enclosing_scope passed
    // to this function.
    nested_scope->DetachFromContaining();
    return Value(this, std::move(nested_scope));
  }
  return Value();
}

LocationRange BlockNode::GetRange() const {
  if (begin_token_.type() != Token::INVALID &&
      end_->value().type() != Token::INVALID) {
    return begin_token_.range().Union(end_->value().range());
  } else if (!statements_.empty()) {
    return statements_[0]->GetRange().Union(
        statements_[statements_.size() - 1]->GetRange());
  }
  return LocationRange();
}

Err BlockNode::MakeErrorDescribing(const std::string& msg,
                                   const std::string& help) const {
  return Err(GetRange(), msg, help);
}

base::Value BlockNode::GetJSONNode() const {
  base::Value dict(CreateJSONNode(kDumpNodeName, GetRange()));
  base::Value statements(base::Value::Type::LIST);
  for (const auto& statement : statements_)
    statements.GetList().push_back(statement->GetJSONNode());
  if (end_)
    dict.SetKey(kJsonEnd, end_->GetJSONNode());

  dict.SetKey(kJsonNodeChild, std::move(statements));

  if (result_mode_ == BlockNode::RETURNS_SCOPE) {
    dict.SetKey(kDumpResultMode, base::Value(kDumpResultModeReturnsScope));
  } else if (result_mode_ == BlockNode::DISCARDS_RESULT) {
    dict.SetKey(kDumpResultMode, base::Value(kDumpResultModeDiscardsResult));
  } else {
    NOTREACHED();
  }

  dict.SetKey(kJsonBeginToken, base::Value(begin_token_.value()));

  return dict;
}

// static
std::unique_ptr<BlockNode> BlockNode::NewFromJSON(const base::Value& value) {
  const std::string& result_mode = value.FindKey(kDumpResultMode)->GetString();
  std::unique_ptr<BlockNode> ret;

  if (result_mode == kDumpResultModeReturnsScope) {
    ret.reset(new BlockNode(BlockNode::RETURNS_SCOPE));
  } else if (result_mode == kDumpResultModeDiscardsResult) {
    ret.reset(new BlockNode(BlockNode::DISCARDS_RESULT));
  } else {
    NOTREACHED();
  }

  DECLARE_CHILD_AS_LIST_OR_FAIL();
  for (const auto& elem : child->GetList()) {
    ret->statements_.push_back(ParseNode::BuildFromJSON(elem));
  }

  ret->begin_token_ =
      Token::ClassifyAndMake(GetBeginLocationFromJSON(value),
                             value.FindKey(kJsonBeginToken)->GetString());
  if (value.FindKey(kJsonEnd)) {
    ret->end_ = EndNode::NewFromJSON(*value.FindKey(kJsonEnd));
  }

  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

// ConditionNode --------------------------------------------------------------

ConditionNode::ConditionNode() = default;

ConditionNode::~ConditionNode() = default;

const ConditionNode* ConditionNode::AsCondition() const {
  return this;
}

Value ConditionNode::Execute(Scope* scope, Err* err) const {
  Value condition_result = condition_->Execute(scope, err);
  if (err->has_error())
    return Value();
  if (condition_result.type() != Value::BOOLEAN) {
    *err = condition_->MakeErrorDescribing(
        "Condition does not evaluate to a boolean value.",
        std::string("This is a value of type \"") +
            Value::DescribeType(condition_result.type()) + "\" instead.");
    err->AppendRange(if_token_.range());
    return Value();
  }

  if (condition_result.boolean_value()) {
    if_true_->Execute(scope, err);
  } else if (if_false_) {
    // The else block is optional.
    if_false_->Execute(scope, err);
  }

  return Value();
}

LocationRange ConditionNode::GetRange() const {
  if (if_false_)
    return if_token_.range().Union(if_false_->GetRange());
  return if_token_.range().Union(if_true_->GetRange());
}

Err ConditionNode::MakeErrorDescribing(const std::string& msg,
                                       const std::string& help) const {
  return Err(if_token_, msg, help);
}

base::Value ConditionNode::GetJSONNode() const {
  base::Value dict = CreateJSONNode(kDumpNodeName, GetRange());
  base::Value child(base::Value::Type::LIST);
  child.GetList().push_back(condition_->GetJSONNode());
  child.GetList().push_back(if_true_->GetJSONNode());
  if (if_false_) {
    child.GetList().push_back(if_false_->GetJSONNode());
  }
  dict.SetKey(kJsonNodeChild, std::move(child));
  return dict;
}

// static
std::unique_ptr<ConditionNode> ConditionNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<ConditionNode>();

  DECLARE_CHILD_AS_LIST_OR_FAIL();
  const base::Value::ListStorage& children = child->GetList();

  ret->if_token_ =
      Token::ClassifyAndMake(GetBeginLocationFromJSON(value), "if");
  ret->condition_ = ParseNode::BuildFromJSON(children[0]);
  ret->if_true_ = BlockNode::NewFromJSON(children[1]);
  if (children.size() > 2) {
    ret->if_false_ = ParseNode::BuildFromJSON(children[2]);
  }
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

// FunctionCallNode -----------------------------------------------------------

FunctionCallNode::FunctionCallNode() = default;

FunctionCallNode::~FunctionCallNode() = default;

const FunctionCallNode* FunctionCallNode::AsFunctionCall() const {
  return this;
}

Value FunctionCallNode::Execute(Scope* scope, Err* err) const {
  return functions::RunFunction(scope, this, args_.get(), block_.get(), err);
}

LocationRange FunctionCallNode::GetRange() const {
  if (function_.type() == Token::INVALID)
    return LocationRange();  // This will be null in some tests.
  if (block_)
    return function_.range().Union(block_->GetRange());
  return function_.range().Union(args_->GetRange());
}

Err FunctionCallNode::MakeErrorDescribing(const std::string& msg,
                                          const std::string& help) const {
  return Err(function_, msg, help);
}

base::Value FunctionCallNode::GetJSONNode() const {
  base::Value dict =
      CreateJSONNode(kDumpNodeName, function_.value(), GetRange());
  base::Value child(base::Value::Type::LIST);
  child.GetList().push_back(args_->GetJSONNode());
  if (block_) {
    child.GetList().push_back(block_->GetJSONNode());
  }
  dict.SetKey(kJsonNodeChild, std::move(child));
  return dict;
}

// static
std::unique_ptr<FunctionCallNode> FunctionCallNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<FunctionCallNode>();

  DECLARE_CHILD_AS_LIST_OR_FAIL();
  const base::Value::ListStorage& children = child->GetList();
  ret->function_ = TokenFromValue(value);
  ret->args_ = ListNode::NewFromJSON(children[0]);
  if (children.size() > 1)
    ret->block_ = BlockNode::NewFromJSON(children[1]);

  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

void FunctionCallNode::SetNewLocation(int line_number) {
  Location func_old_loc = function_.location();
  Location func_new_loc =
      Location(func_old_loc.file(), line_number, func_old_loc.column_number());
  function_.set_location(func_new_loc);

  Location args_old_loc = args_->Begin().location();
  Location args_new_loc =
      Location(args_old_loc.file(), line_number, args_old_loc.column_number());
  const_cast<Token&>(args_->Begin()).set_location(args_new_loc);
  const_cast<Token&>(args_->End()->value()).set_location(args_new_loc);
}

// IdentifierNode --------------------------------------------------------------

IdentifierNode::IdentifierNode() = default;

IdentifierNode::IdentifierNode(const Token& token) : value_(token) {}

IdentifierNode::~IdentifierNode() = default;

const IdentifierNode* IdentifierNode::AsIdentifier() const {
  return this;
}

Value IdentifierNode::Execute(Scope* scope, Err* err) const {
  const Scope* found_in_scope = nullptr;
  const Value* value =
      scope->GetValueWithScope(value_.value(), true, &found_in_scope);
  Value result;
  if (!value) {
    *err = MakeErrorDescribing("Undefined identifier");
    return result;
  }

  if (!EnsureNotReadingFromSameDeclareArgs(this, scope, found_in_scope, err))
    return result;

  result = *value;
  result.set_origin(this);
  return result;
}

LocationRange IdentifierNode::GetRange() const {
  return value_.range();
}

Err IdentifierNode::MakeErrorDescribing(const std::string& msg,
                                        const std::string& help) const {
  return Err(value_, msg, help);
}

base::Value IdentifierNode::GetJSONNode() const {
  return CreateJSONNode(kDumpNodeName, value_.value(), GetRange());
}

// static
std::unique_ptr<IdentifierNode> IdentifierNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<IdentifierNode>();
  ret->set_value(TokenFromValue(value));
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

void IdentifierNode::SetNewLocation(int line_number) {
  Location old = value_.location();
  value_.set_location(Location(old.file(), line_number, old.column_number()));
}

// ListNode -------------------------------------------------------------------

ListNode::ListNode() {}

ListNode::~ListNode() = default;

const ListNode* ListNode::AsList() const {
  return this;
}

Value ListNode::Execute(Scope* scope, Err* err) const {
  Value result_value(this, Value::LIST);
  std::vector<Value>& results = result_value.list_value();
  results.reserve(contents_.size());

  for (const auto& cur : contents_) {
    if (cur->AsBlockComment())
      continue;
    results.push_back(cur->Execute(scope, err));
    if (err->has_error())
      return Value();
    if (results.back().type() == Value::NONE) {
      *err = cur->MakeErrorDescribing("This does not evaluate to a value.",
                                      "I can't do something with nothing.");
      return Value();
    }
  }
  return result_value;
}

LocationRange ListNode::GetRange() const {
  return LocationRange(begin_token_.location(), end_->value().location());
}

Err ListNode::MakeErrorDescribing(const std::string& msg,
                                  const std::string& help) const {
  return Err(begin_token_, msg, help);
}

base::Value ListNode::GetJSONNode() const {
  base::Value dict(CreateJSONNode(kDumpNodeName, GetRange()));
  base::Value child(base::Value::Type::LIST);
  for (const auto& cur : contents_) {
    child.GetList().push_back(cur->GetJSONNode());
  }
  if (end_)
    dict.SetKey(kJsonEnd, end_->GetJSONNode());
  dict.SetKey(kJsonNodeChild, std::move(child));
  dict.SetKey(kJsonBeginToken, base::Value(begin_token_.value()));
  return dict;
}

// static
std::unique_ptr<ListNode> ListNode::NewFromJSON(const base::Value& value) {
  auto ret = std::make_unique<ListNode>();

  DECLARE_CHILD_AS_LIST_OR_FAIL();
  for (const auto& elem : child->GetList()) {
    ret->contents_.push_back(ParseNode::BuildFromJSON(elem));
  }
  ret->begin_token_ =
      Token::ClassifyAndMake(GetBeginLocationFromJSON(value),
                             value.FindKey(kJsonBeginToken)->GetString());
  if (value.FindKey(kJsonEnd)) {
    ret->end_ = EndNode::NewFromJSON(*value.FindKey(kJsonEnd));
  }

  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

template <typename Comparator>
void ListNode::SortList(Comparator comparator) {
  // Partitions first on BlockCommentNodes and sorts each partition separately.
  for (auto sr : GetSortRanges()) {
    bool skip = false;
    for (size_t i = sr.begin; i != sr.end; ++i) {
      // Bails out if any of the nodes are unsupported.
      const ParseNode* node = contents_[i].get();
      if (!node->AsLiteral() && !node->AsIdentifier() && !node->AsAccessor()) {
        skip = true;
        continue;
      }
    }
    if (skip)
      continue;
    // Save the original line number so that we can re-assign ranges. We assume
    // they're contiguous lines because GetSortRanges() does so above. We need
    // to re-assign these line numbers primiarily because `gn format` uses them
    // to determine whether two nodes were initially separated by a blank line
    // or not.
    int start_line = contents_[sr.begin]->GetRange().begin().line_number();
    const ParseNode* original_first = contents_[sr.begin].get();
    std::sort(contents_.begin() + sr.begin, contents_.begin() + sr.end,
              [&comparator](const std::unique_ptr<const ParseNode>& a,
                            const std::unique_ptr<const ParseNode>& b) {
                return comparator(a.get(), b.get());
              });
    // If the beginning of the range had before comments, and the first node
    // moved during the sort, then move its comments to the new head of the
    // range.
    if (original_first->comments() &&
        contents_[sr.begin].get() != original_first) {
      for (const auto& hc : original_first->comments()->before()) {
        const_cast<ParseNode*>(contents_[sr.begin].get())
            ->comments_mutable()
            ->append_before(hc);
      }
      const_cast<ParseNode*>(original_first)
          ->comments_mutable()
          ->clear_before();
    }
    const ParseNode* prev = nullptr;
    for (size_t i = sr.begin; i != sr.end; ++i) {
      const ParseNode* node = contents_[i].get();
      DCHECK(node->AsLiteral() || node->AsIdentifier() || node->AsAccessor());
      int line_number =
          prev ? prev->GetRange().end().line_number() + 1 : start_line;
      if (node->AsLiteral()) {
        const_cast<LiteralNode*>(node->AsLiteral())
            ->SetNewLocation(line_number);
      } else if (node->AsIdentifier()) {
        const_cast<IdentifierNode*>(node->AsIdentifier())
            ->SetNewLocation(line_number);
      } else if (node->AsAccessor()) {
        const_cast<AccessorNode*>(node->AsAccessor())
            ->SetNewLocation(line_number);
      }
      prev = node;
    }
  }
}

void ListNode::SortAsStringsList() {
  // Sorts alphabetically.
  SortList([](const ParseNode* a, const ParseNode* b) {
    std::string_view astr = GetStringRepresentation(a);
    std::string_view bstr = GetStringRepresentation(b);
    return astr < bstr;
  });
}

void ListNode::SortAsTargetsList() {
  // Sorts first relative targets, then absolute, each group is sorted
  // alphabetically.
  SortList([](const ParseNode* a, const ParseNode* b) {
    std::string_view astr = GetStringRepresentation(a);
    std::string_view bstr = GetStringRepresentation(b);
    return std::make_pair(GetDepsCategory(astr), SplitAtFirst(astr, ':')) <
           std::make_pair(GetDepsCategory(bstr), SplitAtFirst(bstr, ':'));
  });
}

// Breaks the ParseNodes of |contents| up by ranges that should be separately
// sorted. In particular, we break at a block comment, or an item that has an
// attached "before" comment and is separated by a blank line from the item
// before it. The assumption is that both of these indicate a separate 'section'
// of a sources block across which items should not be inter-sorted.
std::vector<ListNode::SortRange> ListNode::GetSortRanges() const {
  std::vector<SortRange> ranges;
  const ParseNode* prev = nullptr;
  size_t begin = 0;
  for (size_t i = begin; i < contents_.size(); prev = contents_[i++].get()) {
    if (IsSortRangeSeparator(contents_[i].get(), prev)) {
      if (i > begin) {
        ranges.push_back(SortRange(begin, i));
        // If |i| is an item with an attached comment, then we start the next
        // range at that point, because we want to include it in the sort.
        // Otherwise, it's a block comment which we skip over entirely because
        // we don't want to move or include it in the sort. The two cases are:
        //
        // sources = [
        //   "a",
        //   "b",
        //
        //   #
        //   # This is a block comment.
        //   #
        //
        //   "c",
        //   "d",
        // ]
        //
        // which contains 5 elements, and for which the ranges would be { [0,
        // 2), [3, 5) } (notably excluding 2, the block comment), and:
        //
        // sources = [
        //   "a",
        //   "b",
        //
        //   # This is a header comment.
        //   "c",
        //   "d",
        // ]
        //
        // which contains 4 elements, index 2 containing an attached 'before'
        // comments, and the ranges should be { [0, 2), [2, 4) }.
        if (!contents_[i]->AsBlockComment())
          begin = i;
        else
          begin = i + 1;
      } else {
        // If it was a one item range, just skip over it.
        begin = i + 1;
      }
    }
  }
  if (begin != contents_.size())
    ranges.push_back(SortRange(begin, contents_.size()));
  return ranges;
}

// LiteralNode -----------------------------------------------------------------

LiteralNode::LiteralNode() = default;

LiteralNode::LiteralNode(const Token& token) : value_(token) {}

LiteralNode::~LiteralNode() = default;

const LiteralNode* LiteralNode::AsLiteral() const {
  return this;
}

Value LiteralNode::Execute(Scope* scope, Err* err) const {
  switch (value_.type()) {
    case Token::TRUE_TOKEN:
      return Value(this, true);
    case Token::FALSE_TOKEN:
      return Value(this, false);
    case Token::INTEGER: {
      std::string_view s = value_.value();
      if ((base::StartsWith(s, "0", base::CompareCase::SENSITIVE) &&
           s.size() > 1) ||
          base::StartsWith(s, "-0", base::CompareCase::SENSITIVE)) {
        if (s == "-0")
          *err = MakeErrorDescribing("Negative zero doesn't make sense");
        else
          *err = MakeErrorDescribing("Leading zeros not allowed");
        return Value();
      }
      int64_t result_int;
      if (!base::StringToInt64(s, &result_int)) {
        *err = MakeErrorDescribing("This does not look like an integer");
        return Value();
      }
      return Value(this, result_int);
    }
    case Token::STRING: {
      Value v(this, Value::STRING);
      ExpandStringLiteral(scope, value_, &v, err);
      return v;
    }
    default:
      NOTREACHED();
      return Value();
  }
}

LocationRange LiteralNode::GetRange() const {
  return value_.range();
}

Err LiteralNode::MakeErrorDescribing(const std::string& msg,
                                     const std::string& help) const {
  return Err(value_, msg, help);
}

base::Value LiteralNode::GetJSONNode() const {
  return CreateJSONNode(kDumpNodeName, value_.value(), GetRange());
}

// static
std::unique_ptr<LiteralNode> LiteralNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<LiteralNode>();
  ret->value_ = TokenFromValue(value);
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

void LiteralNode::SetNewLocation(int line_number) {
  Location old = value_.location();
  value_.set_location(Location(old.file(), line_number, old.column_number()));
}

// UnaryOpNode ----------------------------------------------------------------

UnaryOpNode::UnaryOpNode() = default;

UnaryOpNode::~UnaryOpNode() = default;

const UnaryOpNode* UnaryOpNode::AsUnaryOp() const {
  return this;
}

Value UnaryOpNode::Execute(Scope* scope, Err* err) const {
  Value operand_value = operand_->Execute(scope, err);
  if (err->has_error())
    return Value();
  return ExecuteUnaryOperator(scope, this, operand_value, err);
}

LocationRange UnaryOpNode::GetRange() const {
  return op_.range().Union(operand_->GetRange());
}

Err UnaryOpNode::MakeErrorDescribing(const std::string& msg,
                                     const std::string& help) const {
  return Err(op_, msg, help);
}

base::Value UnaryOpNode::GetJSONNode() const {
  base::Value dict = CreateJSONNode(kDumpNodeName, op_.value(), GetRange());
  base::Value child(base::Value::Type::LIST);
  child.GetList().push_back(operand_->GetJSONNode());
  dict.SetKey(kJsonNodeChild, std::move(child));
  return dict;
}

// static
std::unique_ptr<UnaryOpNode> UnaryOpNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<UnaryOpNode>();
  ret->op_ = TokenFromValue(value);
  DECLARE_CHILD_AS_LIST_OR_FAIL();
  ret->operand_ = ParseNode::BuildFromJSON(child->GetList()[0]);
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

// BlockCommentNode ------------------------------------------------------------

BlockCommentNode::BlockCommentNode() = default;

BlockCommentNode::~BlockCommentNode() = default;

const BlockCommentNode* BlockCommentNode::AsBlockComment() const {
  return this;
}

Value BlockCommentNode::Execute(Scope* scope, Err* err) const {
  return Value();
}

LocationRange BlockCommentNode::GetRange() const {
  return comment_.range();
}

Err BlockCommentNode::MakeErrorDescribing(const std::string& msg,
                                          const std::string& help) const {
  return Err(comment_, msg, help);
}

base::Value BlockCommentNode::GetJSONNode() const {
  std::string escaped;
  return CreateJSONNode(kDumpNodeName, comment_.value(), GetRange());
}

// static
std::unique_ptr<BlockCommentNode> BlockCommentNode::NewFromJSON(
    const base::Value& value) {
  auto ret = std::make_unique<BlockCommentNode>();
  ret->comment_ = Token(GetBeginLocationFromJSON(value), Token::BLOCK_COMMENT,
                        value.FindKey(kJsonNodeValue)->GetString());
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}

// EndNode ---------------------------------------------------------------------

EndNode::EndNode(const Token& token) : value_(token) {}

EndNode::~EndNode() = default;

const EndNode* EndNode::AsEnd() const {
  return this;
}

Value EndNode::Execute(Scope* scope, Err* err) const {
  return Value();
}

LocationRange EndNode::GetRange() const {
  return value_.range();
}

Err EndNode::MakeErrorDescribing(const std::string& msg,
                                 const std::string& help) const {
  return Err(value_, msg, help);
}

base::Value EndNode::GetJSONNode() const {
  return CreateJSONNode(kDumpNodeName, value_.value(), GetRange());
}

// static
std::unique_ptr<EndNode> EndNode::NewFromJSON(const base::Value& value) {
  auto ret = std::make_unique<EndNode>(TokenFromValue(value));
  GetCommentsFromJSON(ret.get(), value);
  return ret;
}
