// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_PACKAGE_H_
#define LIBIPP_IPP_PACKAGE_H_

#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_export.h"  // NOLINT(build/include)

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ipp {

class Group;

// This class represents IPP frame. It is container for Groups that represent
// IPP attributes groups (like operation-attributes). Groups in a single Package
// must have unique tags (names).
class IPP_EXPORT Package {
 public:
  // destructor
  virtual ~Package();

  // Returns vector with groups in the schema, all pointers are != nullptr.
  virtual std::vector<Group*> GetKnownGroups() { return {}; }

  // Returns vector of all groups in the package, all pointers are != nullptr.
  // Returned vector = GetKnownGroups() + vector with unknown groups.
  std::vector<Group*> GetAllGroups();

  // Returns pointer to the group with given tag.
  // Returns nullptr if there is no group with given tag in the package.
  Group* GetGroup(GroupTag);

  // Adds a new group to the package and returns pointer to it or nullptr if
  // a group with given tag already exists in the package.
  Group* AddUnknownGroup(GroupTag, bool is_a_set);

  // Returns reference to payload (e.g. document to print), empty vector means
  // no payload.
  std::vector<uint8_t>& Data() { return data_; }

 protected:
  Package() = default;

 private:
  // Copy/move/assign constructors/operators are not available.
  Package(const Package&) = delete;
  Package(Package&&) = delete;
  Package& operator=(const Package&) = delete;
  Package& operator=(Package&&) = delete;

  std::vector<Group*> unknown_groups_;
  std::vector<uint8_t> data_;
};

// Defined later in this file.
class Collection;

// Base class represents single IPP attribute group or a sequence of the
// same IPP attribute groups. Single instance of IPP attribute groups is
// represented by Collection object.
class IPP_EXPORT Group {
 public:
  virtual ~Group() = default;

  // Returns tag of the group.
  GroupTag GetName() const { return name_; }

  // Returns true if it is a sequence of IPP groups (Collections) or false
  // if it is a single IPP group (one Collection).
  bool IsASet() const { return is_a_set_; }

  // Returns the current number of elements (IPP groups) in the Group.
  // (IsASet() == false) => always returns 1.
  virtual size_t GetSize() const = 0;

  // Resizes a sequence of IPP groups. Does nothing if (IsASet() == false).
  virtual void Resize(size_t) = 0;

  // Returns a pointer to underlying collection, representing one of the IPP
  // groups. Returns nullptr <=> (index >= GetSize()).
  virtual Collection* GetCollection(size_t index = 0) = 0;

 protected:
  Group(GroupTag name, bool is_a_set) : name_(name), is_a_set_(is_a_set) {}

 private:
  GroupTag name_;
  bool is_a_set_;
};

// Final class for Groups, represents Group with single IPP attribute group
// defined in the schema. Template parameter TCollection must be a class
// derived from Collection and defines the structure of the group.
template <class TCollection>
class SingleGroup : public Group, public TCollection {
 public:
  explicit SingleGroup(GroupTag name) : Group(name, false) {}

  // Implementation of Group API
  size_t GetSize() const override { return 1; }
  void Resize(size_t) override {}
  Collection* GetCollection(size_t index = 0) override {
    if (index == 0)
      return this;
    return nullptr;
  }
};

// Final class for Group, represents sequence of IPP attribute groups with
// the same tag and defined in the schema. Template parameter TCollection is
// a class derived from Collection and defines the structure of a single group.
template <class TCollection>
class SetOfGroups : public Group {
 public:
  explicit SetOfGroups(GroupTag name) : Group(name, true) {}
  virtual ~SetOfGroups() {
    for (auto g : groups_)
      delete g;
  }

  // Implementation of Group API.
  size_t GetSize() const override { return groups_.size(); }
  void Resize(size_t new_size) override {
    while (groups_.size() < new_size)
      groups_.push_back(new TCollection);
    while (groups_.size() > new_size) {
      delete groups_.back();
      groups_.pop_back();
    }
  }
  Collection* GetCollection(size_t index = 0) override {
    if (index >= groups_.size())
      return nullptr;
    return groups_[index];
  }

  // Operator [] in specialized API, returns one of TCollection from the
  // sequence. If index is out of range, the vector is resized to (index+1).
  TCollection& operator[](size_t index) {
    if (index >= groups_.size())
      Resize(index + 1);
    return *(groups_[index]);
  }

 private:
  std::vector<TCollection*> groups_;
};

// Final class for Group, represents Group not defined in the schema.
class UnknownGroup : public Group {
 public:
  UnknownGroup(GroupTag name, bool is_a_set);
  virtual ~UnknownGroup();

  // Implementation of Group API.
  size_t GetSize() const override { return groups_.size(); }
  void Resize(size_t new_size) override;
  Collection* GetCollection(size_t index = 0) override {
    if (index >= groups_.size())
      return nullptr;
    return groups_[index];
  }

 private:
  std::vector<Collection*> groups_;
};

// These are defined in ipp_attribute.h.
class Attribute;
enum class AttrType : uint8_t;

// Base class for all IPP collections. Collections is like struct filled with
// Attributes. Each attribute in Collection must have unique name.
class IPP_EXPORT Collection {
 public:
  Collection() = default;
  virtual ~Collection();

  // Method to implement in derived class, returns attributes from schema.
  // There is no nullptrs in the returned vector.
  virtual std::vector<Attribute*> GetKnownAttributes() { return {}; }

  // Returns all attributes in the collection.
  // Returned vector = GetKnownAttributes() + unknown attributes.
  // Unknown attributes are in the order they were added to the collection.
  // There is no nullptrs in the returned vector.
  std::vector<Attribute*> GetAllAttributes();

  // Methods return attribute by name. Methods return nullptr <=> the collection
  // has no attribute with this name.
  Attribute* GetAttribute(AttrName);
  Attribute* GetAttribute(const std::string& name);

  // Adds new attribute to the collection. Returns nullptr <=> an attribute
  // with this name already exists in the collection or given name/type are
  // incorrect.
  Attribute* AddUnknownAttribute(const std::string& name,
                                 bool is_a_set,
                                 AttrType type);

  // Clears values and states of all attributes.
  // Calls the following methods for member attributes:
  // * Resize(0) for all member attributes being a set (IsASet() == true).
  // * SetState(unset) for all member attributes.
  // * Recursively ResetAllAttributes() for all Collections stored in member
  //   attributes.
  // Note: Does NOT remove Unknown Attributes from Collection.
  void ResetAllAttributes();

 private:
  // Copy/move/assign constructors/operators are forbidden.
  Collection(const Collection&) = delete;
  Collection(Collection&&) = delete;
  Collection& operator=(const Collection&) = delete;
  Collection& operator=(Collection&&) = delete;

  std::vector<std::string> unknown_attrs_names_;
  std::map<std::string, Attribute*> unknown_attrs_;
};

}  // namespace ipp

#endif  //  LIBIPP_IPP_PACKAGE_H_
