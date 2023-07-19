// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_ATTRIBUTE_H_
#define LIBIPP_IPP_ATTRIBUTE_H_

#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "colls_view.h"
#include "ipp_enums.h"
#include "ipp_export.h"

namespace ipp {

// Forward declaration
enum class Code;

// Values of ValueTag enum are copied from IPP specification; this is why
// they do not follow the standard naming rule.
// ValueTag defines type of an attribute. It is also called as `syntax` in the
// IPP specification. All valid tags are listed below. Values of attributes with
// these tags are mapped to C++ types.
enum class ValueTag : uint8_t {
  // 0x00-0x0f are invalid.
  // 0x10-0x1f are Out-of-Band tags. Attributes with this tag have no values.
  // All tags from the range 0x10-0x1f are valid.
  unsupported = 0x10,       // [rfc8010]
  unknown = 0x12,           // [rfc8010]
  no_value = 0x13,          // [rfc8010]
  not_settable = 0x15,      // [rfc3380]
  delete_attribute = 0x16,  // [rfc3380]
  admin_define = 0x17,      // [rfc3380]
  // 0x20-0x2f represents integer types.
  // Only the following tags are valid. They map to int32_t.
  integer = 0x21,
  boolean = 0x22,  // maps to both int32_t and bool.
  enum_ = 0x23,
  // 0x30-0x3f are called "octetString types". They map to dedicated types.
  // Only the following tags are valid.
  octetString = 0x30,       // maps to std::string
  dateTime = 0x31,          // maps to DateTime
  resolution = 0x32,        // maps to Resolution
  rangeOfInteger = 0x33,    // maps to RangeOfInteger
  collection = 0x34,        // = begCollection tag [rfc8010], maps to Collection
  textWithLanguage = 0x35,  // maps to StringWithLanguage
  nameWithLanguage = 0x36,  // maps to StringWithLanguage
  // 0x40-0x5f represents 'character-string values'. They map to std::string.
  // All tags from the ranges 0x40-0x49 and 0x4b-0x5f are valid.
  textWithoutLanguage = 0x41,
  nameWithoutLanguage = 0x42,
  keyword = 0x44,
  uri = 0x45,
  uriScheme = 0x46,
  charset = 0x47,
  naturalLanguage = 0x48,
  mimeMediaType = 0x49
  // memberAttrName = 0x4a is invalid.
  // 0x60-0xff are invalid.
};

// Is valid Out-of-Band tag (0x10-0x1f).
constexpr bool IsOutOfBand(ValueTag tag) {
  return (tag >= static_cast<ValueTag>(0x10) &&
          tag <= static_cast<ValueTag>(0x1f));
}
// Is valid integer type (0x21-0x23).
constexpr bool IsInteger(ValueTag tag) {
  return (tag >= static_cast<ValueTag>(0x21) &&
          tag <= static_cast<ValueTag>(0x23));
}
// Is valid character-string type (0x40-0x5f without 0x4a).
constexpr bool IsString(ValueTag tag) {
  return (tag >= static_cast<ValueTag>(0x40) &&
          tag <= static_cast<ValueTag>(0x5f) &&
          tag != static_cast<ValueTag>(0x4a));
}
// Is valid tag.
constexpr bool IsValid(ValueTag tag) {
  return (IsOutOfBand(tag) || IsInteger(tag) || IsString(tag) ||
          (tag >= static_cast<ValueTag>(0x30) &&
           tag <= static_cast<ValueTag>(0x36)));
}

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
inline bool operator==(const StringWithLanguage& v1,
                       const StringWithLanguage& v2) {
  return (v1.language == v2.language) && (v1.value == v2.value);
}
inline bool operator!=(const StringWithLanguage& v1,
                       const StringWithLanguage& v2) {
  return !(v1 == v2);
}

// Represents resolution type from [rfc8010].
struct Resolution {
  int32_t xres = 0;
  int32_t yres = 0;
  enum Units : int8_t {
    kDotsPerInch = 3,
    kDotsPerCentimeter = 4
  } units = kDotsPerInch;
  Resolution() = default;
  Resolution(int32_t size1, int32_t size2, Units units = Units::kDotsPerInch)
      : xres(size1), yres(size2), units(units) {}
};
inline bool operator==(const Resolution& v1, const Resolution& v2) {
  return (v1.xres == v2.xres) && (v1.yres == v2.yres) && (v1.units == v2.units);
}
inline bool operator!=(const Resolution& v1, const Resolution& v2) {
  return !(v1 == v2);
}

// Represents rangeOfInteger type from [rfc8010].
struct RangeOfInteger {
  int32_t min_value = 0;
  int32_t max_value = 0;
  RangeOfInteger() = default;
  RangeOfInteger(int32_t min_value, int32_t max_value)
      : min_value(min_value), max_value(max_value) {}
};
inline bool operator==(const RangeOfInteger& v1, const RangeOfInteger& v2) {
  return (v1.min_value == v2.min_value) && (v1.max_value == v2.max_value);
}
inline bool operator!=(const RangeOfInteger& v1, const RangeOfInteger& v2) {
  return !(v1 == v2);
}

// Represents dateTime type from [rfc8010,rfc2579].
struct DateTime {
  uint16_t year = 2000;
  uint8_t month = 1;            // 1..12
  uint8_t day = 1;              // 1..31
  uint8_t hour = 0;             // 0..23
  uint8_t minutes = 0;          // 0..59
  uint8_t seconds = 0;          // 0..60 (60 - leap second :-)
  uint8_t deci_seconds = 0;     // 0..9
  uint8_t UTC_direction = '+';  // '+' / '-'
  uint8_t UTC_hours = 0;        // 0..13
  uint8_t UTC_minutes = 0;      // 0..59
};
inline bool operator==(const DateTime& v1, const DateTime& v2) {
  return (v1.year == v2.year) && (v1.month == v2.month) && (v1.day == v2.day) &&
         (v1.hour == v2.hour) && (v1.minutes == v2.minutes) &&
         (v1.seconds == v2.seconds) && (v1.deci_seconds == v2.deci_seconds) &&
         (v1.UTC_direction == v2.UTC_direction) &&
         (v1.UTC_hours == v2.UTC_hours) && (v1.UTC_minutes == v2.UTC_minutes);
}
inline bool operator!=(const DateTime& v1, const DateTime& v2) {
  return !(v1 == v2);
}

// Functions converting basic types to string. For enums it returns empty
// string if given value is not defined.
LIBIPP_EXPORT std::string_view ToStrView(ValueTag tag);
LIBIPP_EXPORT std::string ToString(bool value);
LIBIPP_EXPORT std::string ToString(int value);
LIBIPP_EXPORT std::string ToString(const Resolution& value);
LIBIPP_EXPORT std::string ToString(const RangeOfInteger& value);
LIBIPP_EXPORT std::string ToString(const DateTime& value);
LIBIPP_EXPORT std::string ToString(const StringWithLanguage& value);

// Functions extracting basic types from string.
// Returns false <=> given pointer is nullptr or given string does not
// represent a correct value.
LIBIPP_EXPORT bool FromString(const std::string& str, bool* value);
LIBIPP_EXPORT bool FromString(const std::string& str, int* value);

// Basic values are stored in attributes as variables of the following types:
enum InternalType : uint8_t {
  kInteger,             // int32_t
  kString,              // std::string
  kStringWithLanguage,  // ipp::StringWithLanguage
  kResolution,          // ipp::Resolution
  kRangeOfInteger,      // ipp::RangeOfInteger
  kDateTime,            // ipp::DateTime
  kCollection           // Collection*
};

class Attribute;
class Collection;

// Helper structure
struct AttrDef {
  ValueTag ipp_type;
  InternalType cc_type;
};

// Base class for all IPP collections. Collections is like struct filled with
// Attributes. Each attribute in Collection must have unique non-empty name.
// Use AddAttr() methods to add new attributes to the collection and GetAttr()
// to get access to the attribute by its name. To iterate over all attributes in
// the collection use iterators, e.g.:
//
//    for (Attribute& attr: collection) { ... }
//    for (const Attribute& attr: collection) { ... }
//
// Attributes inside the collection are always in the same order they were added
// to it. They will also appear in the same order in the resultant frame.
class LIBIPP_EXPORT Collection {
 public:
  class const_iterator;
  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Attribute;
    using difference_type = int;
    using pointer = Attribute*;
    using reference = Attribute&;

    iterator() = default;
    iterator& operator++() {
      ++iter_;
      return *this;
    }
    iterator& operator--() {
      --iter_;
      return *this;
    }
    iterator operator++(int) { return iterator(iter_++); }
    iterator operator--(int) { return iterator(iter_--); }
    Attribute& operator*() { return *(iter_->get()); }
    Attribute* operator->() { return iter_->get(); }
    bool operator==(const iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const iterator& i) const { return iter_ != i.iter_; }
    bool operator==(const const_iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const const_iterator& i) const { return iter_ != i.iter_; }

   private:
    friend class Collection;
    explicit iterator(std::vector<std::unique_ptr<Attribute>>::iterator iter)
        : iter_(iter) {}
    std::vector<std::unique_ptr<Attribute>>::iterator iter_;
  };

  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = const Attribute;
    using difference_type = int;
    using pointer = const Attribute*;
    using reference = const Attribute&;

    const_iterator() = default;
    explicit const_iterator(iterator it) : iter_(it.iter_) {}
    const_iterator& operator=(iterator it) {
      iter_ = it.iter_;
      return *this;
    }
    const_iterator& operator++() {
      ++iter_;
      return *this;
    }
    const_iterator& operator--() {
      --iter_;
      return *this;
    }
    const_iterator operator++(int) { return const_iterator(iter_++); }
    const_iterator operator--(int) { return const_iterator(iter_--); }
    const Attribute& operator*() { return *(iter_->get()); }
    const Attribute* operator->() { return iter_->get(); }
    bool operator==(const iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const iterator& i) const { return iter_ != i.iter_; }
    bool operator==(const const_iterator& i) const { return iter_ == i.iter_; }
    bool operator!=(const const_iterator& i) const { return iter_ != i.iter_; }

   private:
    friend class Collection;
    explicit const_iterator(
        std::vector<std::unique_ptr<Attribute>>::const_iterator iter)
        : iter_(iter) {}
    std::vector<std::unique_ptr<Attribute>>::const_iterator iter_;
  };

  Collection();
  Collection(const Collection&) = delete;
  Collection(Collection&&) = delete;
  Collection& operator=(const Collection&) = delete;
  Collection& operator=(Collection&&) = delete;
  virtual ~Collection();

  // Standard container methods.
  iterator begin() { return iterator(attributes_.begin()); }
  iterator end() { return iterator(attributes_.end()); }
  const_iterator cbegin() const { return const_iterator(attributes_.cbegin()); }
  const_iterator cend() const { return const_iterator(attributes_.cend()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  size_t size() const { return attributes_index_.size(); }
  bool empty() const { return attributes_index_.empty(); }

  // Methods return attribute by name. Methods return an iterator end() <=> the
  // collection has no attributes with this name.
  iterator GetAttr(std::string_view name);
  const_iterator GetAttr(std::string_view name) const;

  // Add a new attribute without values. `tag` must be Out-Of-Band (see ValueTag
  // definition). Possible errors:
  //  * kInvalidName
  //  * kNameConflict
  //  * kInvalidValueTag
  //  * kIncompatibleType  (`tag` is not Out-Of-Band)
  //  * kTooManyAttributes.
  Code AddAttr(const std::string& name, ValueTag tag);

  // Add a new attribute with one or more values. `tag` must be compatible with
  // type of the parameter `value`/`values` according to the following rules:
  //  * int32_t: IsInteger(tag) == true
  //  * std::string: IsString(tag) == true OR tag == octetString
  //  * StringWithLanguage: tag == nameWithLanguage OR tag == textWithLanguage
  //  * DateTime: tag == dateTime
  //  * Resolution: tag == resolution
  //  * RangeOfInteger: tag == rangeOfInteger
  // Possible errors:
  //  * kInvalidName
  //  * kNameConflict
  //  * kInvalidValueTag
  //  * kIncompatibleType  (see the rules above)
  //  * kValueOutOfRange   (the vector is empty or one of the value is invalid)
  //  * kTooManyAttributes.
  Code AddAttr(const std::string& name, ValueTag tag, int32_t value);
  Code AddAttr(const std::string& name, ValueTag tag, const std::string& value);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const StringWithLanguage& value);
  Code AddAttr(const std::string& name, ValueTag tag, DateTime value);
  Code AddAttr(const std::string& name, ValueTag tag, Resolution value);
  Code AddAttr(const std::string& name, ValueTag tag, RangeOfInteger value);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<int32_t>& values);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<std::string>& values);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<StringWithLanguage>& values);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<DateTime>& values);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<Resolution>& values);
  Code AddAttr(const std::string& name,
               ValueTag tag,
               const std::vector<RangeOfInteger>& values);

  // Add a new attribute with one or more values. Tag of the new attribute is
  // deduced from the type of the parameter `value`/`values`.
  // Possible errors:
  //  * kInvalidName
  //  * kNameConflict
  //  * kValueOutOfRange   (the vector is empty)
  //  * kTooManyAttributes.
  Code AddAttr(const std::string& name, bool value);
  Code AddAttr(const std::string& name, int32_t value);
  Code AddAttr(const std::string& name, DateTime value);
  Code AddAttr(const std::string& name, Resolution value);
  Code AddAttr(const std::string& name, RangeOfInteger value);
  Code AddAttr(const std::string& name, const std::vector<bool>& values);
  Code AddAttr(const std::string& name, const std::vector<int32_t>& values);
  Code AddAttr(const std::string& name, const std::vector<DateTime>& values);
  Code AddAttr(const std::string& name, const std::vector<Resolution>& values);
  Code AddAttr(const std::string& name,
               const std::vector<RangeOfInteger>& values);

  // Add a new attribute with one or more collections. The first method creates
  // an attribute with a single collection and returns an iterator to it in the
  // last parameter. The second method creates an attribute with `size`
  // collections and returns a view of them in the last parameters.
  // Possible errors:
  //  * kInvalidName
  //  * kNameConflict
  //  * kValueOutOfRange   (`size` is out of range)
  //  * kTooManyAttributes.
  Code AddAttr(const std::string& name, CollsView::iterator& coll);
  Code AddAttr(const std::string& name, size_t size, CollsView& colls);

 private:
  friend class Attribute;

  // Adds new attribute to the collection. Returns Code::OK <=> an attribute
  // was created. A pointer to the new attribute is saved to `new_attr`.
  Code CreateNewAttribute(const std::string& name,
                          ValueTag type,
                          Attribute*& new_attr);

  // Tries to add a new attribute to the collection and set initial values for
  // it. This function does not check compatibility of `tag` and ApiType. All
  // other constraints are enforced. If `tag` is Out-Of-Band the parameter
  // `values` is ignored.
  template <typename ApiType>
  Code AddAttributeToCollection(const std::string& name,
                                ValueTag tag,
                                const std::vector<ApiType>& values);

  // Stores attributes in the order they are saved in the frame.
  std::vector<std::unique_ptr<Attribute>> attributes_;

  // Indexes attributes by name. Values are indices from `attributes_`.
  std::unordered_map<std::string_view, size_t> attributes_index_;
};

// Base class representing Attribute, contains general API for Attribute.
class LIBIPP_EXPORT Attribute {
 public:
  Attribute(const Attribute&) = delete;
  Attribute(Attribute&&) = delete;
  Attribute& operator=(const Attribute&) = delete;
  Attribute& operator=(Attribute&&) = delete;

  virtual ~Attribute();

  // Returns tag of the attribute.
  ValueTag Tag() const;

  // Returns an attribute's name. It is always a non-empty string.
  std::string_view Name() const;

  // Returns the current number of elements (values or Collections).
  // It returns 0 <=> IsOutOfBand(Tag()).
  size_t Size() const;

  // Resizes the attribute (changes the number of stored values/collections).
  // When (IsOutOfBand(Tag()) or `new_size` equals 0 this method does nothing.
  void Resize(size_t new_size);

  // Retrieves a single value from the attribute and saves it in `value`.
  // Possible errors:
  //  * kIncompatibleType
  //  * kIndexOutOfRange
  // The second parameter must match the type of the attribute, otherwise the
  // code kIncompatibleType is returned. However, there are a couple of
  // exceptions when the underlying value is silently converted to the type of
  // the given parameter. See the table below for a list of silent conversions:
  //
  //      ValueTag       |  target C++ type
  //  -------------------+--------------------
  //       boolean       |      int32_t  (0 or 1)
  //        enum         |      int32_t
  //       integer       |  RangeOfIntegers  (as range [int,int])
  // nameWithoutLanguage | StringWithLanguage  (language is empty)
  // textWithoutLanguage | StringWithLanguage  (language is empty)
  //
  Code GetValue(size_t index, bool& value) const;
  Code GetValue(size_t index, int32_t& value) const;
  Code GetValue(size_t index, std::string& value) const;
  Code GetValue(size_t index, StringWithLanguage& value) const;
  Code GetValue(size_t index, DateTime& value) const;
  Code GetValue(size_t index, Resolution& value) const;
  Code GetValue(size_t index, RangeOfInteger& value) const;

  // Retrieves values from the attribute. They are copied to the given vector.
  // Possible errors:
  //  * kIncompatibleType
  // These methods follow the same rules for types' conversions as GetValue().
  Code GetValues(std::vector<bool>& values) const;
  Code GetValues(std::vector<int32_t>& values) const;
  Code GetValues(std::vector<std::string>& values) const;
  Code GetValues(std::vector<StringWithLanguage>& values) const;
  Code GetValues(std::vector<DateTime>& values) const;
  Code GetValues(std::vector<Resolution>& values) const;
  Code GetValues(std::vector<RangeOfInteger>& values) const;

  // Set new values for the attribute. Previous values are discarded.
  // Possible errors:
  //  * kIncompatibleType
  //  * kValueOutOfRange
  Code SetValues(bool value);
  Code SetValues(int32_t value);
  Code SetValues(const std::string& value);
  Code SetValues(const StringWithLanguage& value);
  Code SetValues(DateTime value);
  Code SetValues(Resolution value);
  Code SetValues(RangeOfInteger value);
  Code SetValues(const std::vector<bool>& values);
  Code SetValues(const std::vector<int32_t>& values);
  Code SetValues(const std::vector<std::string>& values);
  Code SetValues(const std::vector<StringWithLanguage>& values);
  Code SetValues(const std::vector<DateTime>& values);
  Code SetValues(const std::vector<Resolution>& values);
  Code SetValues(const std::vector<RangeOfInteger>& values);

  // Provides access to Collection objects. You can iterate over them in the
  // following ways:
  //   for (Collection& coll: attr.Colls()) {
  //     ...
  //   }
  // or
  //   for (size_t i = 0; i < attr.Colls().size(); ) {
  //     Collection& coll = attr.Colls()[i++];
  //     ...
  //   }
  // (Tag() != collection) <=> attr.Colls().empty()
  CollsView Colls();
  ConstCollsView Colls() const;

 private:
  friend class Collection;

  // Constructor is called from Collection only.
  Attribute(std::string_view name, AttrDef def);

  // Returns the current number of elements (values or Collections).
  // (IsASet() == false) => always returns 0 or 1.
  size_t GetSize() const;

  // Helper template function.
  template <typename ApiType>
  bool SaveValue(size_t index, const ApiType& value);

  // The name of the attribute.
  std::string name_;

  // Defines the type of values stored in the attribute.
  const AttrDef def_;

  // Stores values of the attribute.
  void* values_ = nullptr;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_ATTRIBUTE_H_
