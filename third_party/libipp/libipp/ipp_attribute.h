// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_ATTRIBUTE_H_
#define LIBIPP_IPP_ATTRIBUTE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_export.h"  // NOLINT(build/include)

namespace ipp {

// Represents the current state of the attribute:
// set/unset or one of the out-of-band values.
// "unset" means that the attribute is not included in a IPP frame.
enum class AttrState : uint8_t {
  unset = 0x00,             // internal
  set = 0x01,               // internal
  unsupported = 0x10,       // [rfc8010]
  unknown = 0x12,           // [rfc8010]
  novalue_ = 0x13,          // [rfc8010]
  not_settable = 0x15,      // [rfc3380]
  delete_attribute = 0x16,  // [rfc3380]
  admin_define = 0x17       // [rfc3380]
};

// Represents types of values hold by attributes (see [rfc8010]).
// "collection" means that the attribute has class Collection as a value.
enum class AttrType : uint8_t {
  integer = 0x21,
  boolean = 0x22,
  enum_ = 0x23,
  octetString = 0x30,
  dateTime = 0x31,
  resolution = 0x32,
  rangeOfInteger = 0x33,
  collection = 0x34,  // use begCollection tag value [rfc8010]
  text = 0x35,        // use textWithLanguage tag value [rfc8010]
  name = 0x36,        // use nameWithLanguage tag value [rfc8010]
  keyword = 0x44,
  uri = 0x45,
  uriScheme = 0x46,
  charset = 0x47,
  naturalLanguage = 0x48,
  mimeMediaType = 0x49
};

// It is used to hold name and text values (see [rfc8010]).
// If language == "" it represents nameWithoutLanguage or textWithoutLanguage,
// otherwise it represents nameWithLanguage or textWithLanguage.
struct StringWithLanguage {
  std::string value = "";
  std::string language = "";
  StringWithLanguage() = default;
  StringWithLanguage(const std::string& value, const std::string& language)
      : value(value), language(language) {}
  explicit StringWithLanguage(const std::string& value) : value(value) {}
  explicit StringWithLanguage(std::string&& value) : value(value) {}
  operator std::string() const { return value; }
};

// Represents resolution type from [rfc8010].
struct Resolution {
  int size1 = 0;
  int size2 = 0;
  enum Units { kDotsPerInch = 3, kDotsPerCentimeter = 4 } units = kDotsPerInch;
  Resolution() = default;
  Resolution(int size1, int size2, Units units = Units::kDotsPerInch)
      : size1(size1), size2(size2), units(units) {}
};

// Represents rangeOfInteger type from [rfc8010].
struct RangeOfInteger {
  int min_value = 0;
  int max_value = 0;
  RangeOfInteger() = default;
  RangeOfInteger(int min_value, int max_value)
      : min_value(min_value), max_value(max_value) {}
};

// Represents dateTime type from [rfc8010,rfc2579].
struct DateTime {
  uint16_t year = 2000;
  uint8_t month = 1;            // 1..12
  uint8_t day = 1;              // 1..31
  uint8_t hour = 0;             // 0..23
  uint8_t minutes = 0;          // 0..59
  uint8_t seconds = 0;          //  0..60 (60 - leap second :-)
  uint8_t deci_seconds = 0;     // 0..9
  uint8_t UTC_direction = '+';  // '+' / '-'
  uint8_t UTC_hours = 0;        // 0..13
  uint8_t UTC_minutes = 0;      //  0..59
};

// Functions converting basic types to string. For enums it returns empty
// string if given value is not defined.
IPP_EXPORT std::string ToString(AttrState value);
IPP_EXPORT std::string ToString(AttrType value);
IPP_EXPORT std::string ToString(bool value);
IPP_EXPORT std::string ToString(int value);
IPP_EXPORT std::string ToString(const Resolution& value);
IPP_EXPORT std::string ToString(const RangeOfInteger& value);
IPP_EXPORT std::string ToString(const DateTime& value);

// Functions extracting basic types from string.
// Returns false <=> given pointer is nullptr or given string does not
// represent a correct value.
IPP_EXPORT bool FromString(const std::string& str, bool* value);
IPP_EXPORT bool FromString(const std::string& str, int* value);

// It is defined in ipp_package.h. Represents struct-like entity with
// attributes, where each attribute must have unique name.
class Collection;

// Base class representing Attribute, contains general API for Attribute.
class IPP_EXPORT Attribute {
 public:
  virtual ~Attribute() = default;

  // Returns a type of the attribute.
  AttrType GetType() const { return type_; }

  // Returns a state of an attribute. Default state is always AttrState::unset,
  // setting any value with SetValues(...) switches the state to AttrState::set.
  // State can be also set by hand with SetState() method.
  // If (GetType() != collection): returns a state value saved in the object.
  // If (GetType() == collection): the following algorithm is used:
  // 1. If the attribute's state was set directly to one of out-of-band values
  //    (values different that AttrState::unset and AttrState::set), this value
  //    is returned.
  // 2. If (GetState() == unset) for all members in all Collections in the
  //    attribute, the value AttrState::unset is returned (recursive check).
  // 3. Returns AttrState::set.
  AttrState GetState();

  // Sets state of the attribute.
  // If (GetType() != collection): it sets directly the state in the object.
  // If (GetType() == collection): the following rules apply:
  // * If (new_state == unset), it clears out-of-band value from the object
  //   and calls recursively SetState(unset) for all members in all
  //   Collections in the attribute.
  // * If (new_state == set), it only clears out-of-band value from the object.
  // * In all other cases it just sets directly the given out-of-band value in
  //   the object.
  void SetState(AttrState new_state);

  // Returns true if the attribute is a set, false if it is a single value.
  bool IsASet() const { return is_a_set_; }

  // Returns enum value corresponding to attributes name. If the name has
  // no corresponding AttrName value, it returns AttrName::_unknown.
  AttrName GetNameAsEnum() const { return name_; }

  // Returns an attribute's name as a non-empty string.
  virtual std::string GetName() const { return ToString(name_); }

  // Returns the current number of elements (values or Collections).
  // (IsASet() == false) => always returns 1.
  virtual size_t GetSize() const = 0;

  // Resizes a set of values or Collections if IsASet() == true.
  // (IsASet() == false) => does nothing.
  virtual void Resize(size_t) = 0;

  // Retrieves a value from an attribute, returns true for success and
  // false if the index is out of range or the value cannot be converted
  // to given variable (in this case, the given variable is not modified).
  // (val == nullptr) => does nothing and returns false.
  // (GetType() == collection) => does nothing and returns false.
  virtual bool GetValue(std::string* val, size_t index = 0) const {
    return false;
  }
  virtual bool GetValue(StringWithLanguage* val, size_t index = 0) const {
    return false;
  }
  virtual bool GetValue(int* val, size_t index = 0) const { return false; }
  virtual bool GetValue(Resolution* val, size_t index = 0) const {
    return false;
  }
  virtual bool GetValue(RangeOfInteger* val, size_t index = 0) const {
    return false;
  }
  virtual bool GetValue(DateTime* val, size_t index = 0) const { return false; }

  // Stores a value in given attribute's element. If the attribute is a set
  // and given index is out of range, the underlying container is resized.
  // Returns true for success and false if given value cannot be converted
  // to internal variable or one of the following rules applies:
  // (GetType() == collection) => does nothing and returns false.
  // (IsASet() == false && index != 0) => does nothing and returns false.
  virtual bool SetValue(const std::string& val, size_t index = 0) {
    return false;
  }
  virtual bool SetValue(const StringWithLanguage& val, size_t index = 0) {
    return false;
  }
  virtual bool SetValue(const int& val, size_t index = 0) { return false; }
  virtual bool SetValue(const Resolution& val, size_t index = 0) {
    return false;
  }
  virtual bool SetValue(const RangeOfInteger& val, size_t index = 0) {
    return false;
  }
  virtual bool SetValue(const DateTime& val, size_t index = 0) { return false; }

  // Returns a pointer to Collection.
  // (GetType() != collection || index >= GetSize()) <=> returns nullptr.
  virtual Collection* GetCollection(size_t index = 0) { return nullptr; }

 protected:
  // Constructor is available from derived classes only.
  Attribute(AttrName name, bool is_a_set, AttrType type)
      : type_(type), name_(name), is_a_set_(is_a_set) {}

  const AttrType type_;
  const AttrName name_;
  const bool is_a_set_;
  AttrState state_ = AttrState::unset;

 private:
  // Copy/move/assign constructors/operators are forbidden.
  Attribute(const Attribute&) = delete;
  Attribute(Attribute&&) = delete;
  Attribute& operator=(const Attribute&) = delete;
  Attribute& operator=(Attribute&&) = delete;
};

// Basic values are stored in attributes as variables of the following types:
enum InternalType : uint8_t {
  kInteger,             // int
  kString,              // std::string
  kStringWithLanguage,  // ipp::StringWithLanguage
  kResolution,          // ipp::Resolution
  kRange,               // ipp::RangeOfIntegers
  kDateTime             // ipp::DateTime
};

// General class for storing basic values, provides implementation for methods
// GetValue(...) and SetValue(...). It is not suppose to be used directly.
class IPP_EXPORT ValueAttribute : public Attribute {
 public:
  virtual ~ValueAttribute();

  // Implementation of general API from Attribute.
  size_t GetSize() const override;
  void Resize(size_t) override;
  bool GetValue(std::string* val, size_t index = 0) const override;
  bool GetValue(StringWithLanguage* val, size_t index = 0) const override;
  bool GetValue(int* val, size_t index = 0) const override;
  bool GetValue(Resolution* val, size_t index = 0) const override;
  bool GetValue(RangeOfInteger* val, size_t index = 0) const override;
  bool GetValue(DateTime* val, size_t index = 0) const override;
  bool SetValue(const std::string& val, size_t index = 0) override;
  bool SetValue(const StringWithLanguage& val, size_t index = 0) override;
  bool SetValue(const int& val, size_t index = 0) override;
  bool SetValue(const Resolution& val, size_t index = 0) override;
  bool SetValue(const RangeOfInteger& val, size_t index = 0) override;
  bool SetValue(const DateTime& val, size_t index = 0) override;

 protected:
  // Constructor is available from derived classes only.
  ValueAttribute(AttrName, bool is_a_set, AttrType, InternalType);

 private:
  InternalType cpp_type_;
  union Data {
    std::vector<int> as_int;
    std::vector<std::string> as_string;
    std::vector<Resolution> as_resolution;
    std::vector<RangeOfInteger> as_ranges;
    std::vector<DateTime> as_datetime;
    std::vector<StringWithLanguage> as_string_with_language;
    // We have to provide constructor and destructor because union's
    // elements are not trivial. The union is initialized/deleted in
    // constructor/destructor of the class.
    IPP_PRIVATE Data() {}
    IPP_PRIVATE ~Data() {}
  } data_;
};

// These templates convert the type from specialized API to the internal type
// used to store values. Default is integer because it is used for all enum
// types.
template <typename TType>
struct AsInternalType {
  static const InternalType value = InternalType::kInteger;
  typedef int Type;
};
template <>
struct AsInternalType<std::string> {
  static const InternalType value = InternalType::kString;
  typedef std::string Type;
};
template <>
struct AsInternalType<Resolution> {
  static const InternalType value = InternalType::kResolution;
  typedef Resolution Type;
};
template <>
struct AsInternalType<RangeOfInteger> {
  static const InternalType value = InternalType::kRange;
  typedef RangeOfInteger Type;
};
template <>
struct AsInternalType<DateTime> {
  static const InternalType value = InternalType::kDateTime;
  typedef DateTime Type;
};
template <>
struct AsInternalType<StringWithLanguage> {
  static const InternalType value = InternalType::kStringWithLanguage;
  typedef StringWithLanguage Type;
};

// Final class for Attribute, represents single value from the schema.
// Template parameter defines type of the value.
template <typename TValue>
class SingleValue : public ValueAttribute {
 public:
  SingleValue(AttrName name, AttrType type)
      : ValueAttribute(name, false, type, AsInternalType<TValue>::value) {}

  // Specialized API
  void Set(const TValue& val) {
    SetValue(static_cast<typename AsInternalType<TValue>::Type>(val));
  }
  TValue Get() const {
    typename AsInternalType<TValue>::Type val;
    GetValue(&val);
    return static_cast<TValue>(val);
  }
};

// Specialization of the template above for StringWithLanguage to add
// Set(std::string).
template <>
class SingleValue<StringWithLanguage> : public ValueAttribute {
 public:
  SingleValue(AttrName name, AttrType type)
      : ValueAttribute(
            name, false, type, AsInternalType<StringWithLanguage>::value) {}

  // Specialized API
  void Set(const StringWithLanguage& val) {
    SetValue(static_cast<StringWithLanguage>(val));
  }
  void Set(const std::string& val) {
    SetValue(static_cast<StringWithLanguage>(val));
  }
  StringWithLanguage Get() const {
    StringWithLanguage val;
    GetValue(&val);
    return val;
  }
};

// Final class for Attribute, represents sets of values.
// Template parameter defines type of a single value.
template <typename TValue>
class SetOfValues : public ValueAttribute {
 public:
  SetOfValues(AttrName name, AttrType type)
      : ValueAttribute(name, true, type, AsInternalType<TValue>::value) {}

  // Specialized API
  void Set(const std::vector<TValue>& vals) {
    Resize(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], i);
  }
  std::vector<TValue> Get() const {
    std::vector<TValue> vals(GetSize());
    for (size_t i = 0; i < vals.size(); ++i) {
      typename AsInternalType<TValue>::Type val;
      GetValue(&val, i);
      vals[i] = static_cast<TValue>(val);
    }
    return vals;
  }
  void Add(const std::vector<TValue>& vals) {
    const size_t old_size = GetSize();
    Resize(old_size + vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], old_size + i);
  }
};

// Specialization of the template above for StringWithLanguage to add
// Set/Add(std::string).
template <>
class SetOfValues<StringWithLanguage> : public ValueAttribute {
 public:
  SetOfValues(AttrName name, AttrType type)
      : ValueAttribute(
            name, true, type, AsInternalType<StringWithLanguage>::value) {}

  // Specialized API
  void Set(const std::vector<StringWithLanguage>& vals) {
    Resize(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], i);
  }
  void Set(const std::vector<std::string>& vals) {
    Resize(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], i);
  }
  std::vector<StringWithLanguage> Get() const {
    std::vector<StringWithLanguage> vals(GetSize());
    for (size_t i = 0; i < vals.size(); ++i) {
      StringWithLanguage val;
      GetValue(&val, i);
      vals[i] = val;
    }
    return vals;
  }
  void Add(const std::vector<StringWithLanguage>& vals) {
    const size_t old_size = GetSize();
    Resize(old_size + vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], old_size + i);
  }
  void Add(const std::vector<std::string>& vals) {
    const size_t old_size = GetSize();
    Resize(old_size + vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], old_size + i);
  }
};

// Final class for Attribute, represents sets of values that can contain names
// outside the schema.
template <typename TValue>
class OpenSetOfValues : public ValueAttribute {
 public:
  OpenSetOfValues(AttrName name, AttrType type)
      : ValueAttribute(name, true, type, InternalType::kString) {}

  // Specialized API
  void Set(const std::vector<std::string>& vals) {
    Resize(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], i);
  }
  void Set(const std::vector<TValue>& vals) {
    Resize(vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(ToString(vals[i]), i);
  }
  std::vector<std::string> Get() const {
    std::vector<std::string> vals(GetSize());
    for (size_t i = 0; i < vals.size(); ++i)
      GetValue(&(vals[i]), i);
    return vals;
  }
  void Add(const std::vector<std::string>& vals) {
    const size_t old_size = GetSize();
    Resize(old_size + vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(vals[i], old_size + i);
  }
  void Add(const std::vector<TValue>& vals) {
    const size_t old_size = GetSize();
    Resize(old_size + vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
      SetValue(ToString(vals[i]), old_size + i);
  }
};

// Final class for Attribute, represents attribute not defined in the schema.
class IPP_EXPORT UnknownValueAttribute : public ValueAttribute {
 public:
  UnknownValueAttribute(const std::string& name, bool is_a_set, AttrType type);
  std::string GetName() const override { return str_name_; }

 private:
  std::string str_name_;
};

// Final class for Attribute, represents single IPP collection.
// Template parameter is a class derived from Collection and defines
// the structure.
template <class TCollection>
class SingleCollection : public Attribute, public TCollection {
 public:
  explicit SingleCollection(AttrName name)
      : Attribute(name, false, AttrType::collection) {}

  // Implementation of general API from Attribute.
  size_t GetSize() const override { return 1; }
  void Resize(size_t) override {}
  Collection* GetCollection(size_t index = 0) override {
    if (index == 0)
      return this;
    return nullptr;
  }
};

// Final class for Attribute, represents set of collections from IPP spec.
// Template parameter is a class derived from Collection and defines
// the structure of a single collection.
template <class TCollection>
class SetOfCollections : public Attribute {
 public:
  explicit SetOfCollections(AttrName name)
      : Attribute(name, true, AttrType::collection) {}

  // Implementation of general API from Attribute.
  size_t GetSize() const override { return collections_.size(); }
  void Resize(size_t new_size) override {
    while (collections_.size() < new_size)
      collections_.push_back(std::make_unique<TCollection>());
    collections_.resize(new_size);
  }
  Collection* GetCollection(size_t index = 0) override {
    if (index >= collections_.size())
      return nullptr;
    return collections_[index].get();
  }

  // Specialized API
  // If index is out of range, the vector is resized to (index+1).
  TCollection& operator[](size_t index) {
    if (index >= collections_.size())
      Resize(index + 1);
    return *(collections_[index]);
  }

 private:
  std::vector<std::unique_ptr<TCollection>> collections_;
};

class IPP_EXPORT UnknownCollectionAttribute : public Attribute {
 public:
  UnknownCollectionAttribute(const std::string& name, bool is_a_set);

  // Implementation of general API from Attribute.
  std::string GetName() const override { return str_name_; }
  size_t GetSize() const override { return collections_.size(); }
  void Resize(size_t new_size) override;
  Collection* GetCollection(size_t index = 0) override {
    if (index >= collections_.size())
      return nullptr;
    return collections_[index].get();
  }

 private:
  std::string str_name_;
  std::vector<std::unique_ptr<Collection>> collections_;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_ATTRIBUTE_H_
