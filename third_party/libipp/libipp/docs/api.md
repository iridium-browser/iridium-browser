# API

[TOC]

## Example

This example shows how to use **Get-Printer-Attributes** operation to get value
of **printer-make-and-model** from the printer:

```c++
std::string url = "ipp://my.printer/ipp";
std::vector<uint8_t> frame; // this is a buffer for IPP frames

ipp::Frame request(ipp::Operation::Get_Printer_Attributes);
request.Groups(ipp::GroupTag::operation_attributes)[0].AddAttr("printer-uri", ipp::ValueTag::uri, url);

frame = ipp::BuildBinaryFrame(request)

// ... Now you have to send the content of frame as a payload in HTTP POST
// ... request with content-type set to application/ipp.
// ... Then you have to retrieve a payload from obtained HTTP response and
// ... save it to the buffer frame. Remember to check for HTTP errors.

ipp::SimpleParserLog log;
ipp::Frame response = ipp::Parse(frame.data(), frame.size(), log);
if (!log.CriticalErrors().empty()) {
  // critical parsing error
  return false;
}
auto coll = response.Groups(ipp::GroupTag::operation_attributes)[0];
auto it = coll.GetAttr("printer-make-and-model");
if (it == coll.end()) {
  std::cout << "Not included in the response :(\n";
} else {
  std::string mam;
  it->GetValue(&mam);
  std::cout << mam << "\n";
}
```

In `ipp_test.cc` in tests `example1`-`example9` you can find more examples how
to build IPP frames.

> NOTE: The following attributes from the group **operation-attributes** are set
> automatically:
>
> *   **attributes-charset**: set to `"utf-8"`
> *   **attributes-natural-language**: set to `"en-us"`.


> WARNING: If the method `SimpleParserLog::CriticalErrors()` returns a non-empty
> vector it means that the parser did not finish parsing because of an error
> in the input data. In this case, returned `Frame` object contains only parsed
> part of the frame; in the worst-case scenario the returned object may be
> empty. If the parser was able to parse the whole frame the method
> `SimpleParserLog::CriticalErrors()` returns an empty vector. Even then, the
> method `SimpleParserLog::Errors()` can return a non-empty vector with some
> errors. This is because the parser can recover from many local errors in IPP
> frames by skipping incorrect values or attributes. These non-critical errors
> are also logged to `ParserLog`, but they do not abort parsing. Unfortunately,
> there is significant amount of printers that return IPP frames with this kind
> of non-critical errors.

## Parser

The parser function is declared in the header `parser.h`.

```c++
// Parse the frame of `size` bytes saved in `buffer`. Errors detected by the
// parser are saved to `log`. The returned object is always valid. In the
// worst case scenario (nothing was parsed), the returned frame is empty
// and has all basic parameters set to zeroes like after Frame() constructor.
// In case of parsing errors, some groups or attributes from the input buffer
// may be omitted. You should examine the content of ParserLog to make sure
// that the whole input frame was parsed.
Frame Parse(const uint8_t* buffer, size_t size, ParserLog& log);
```

The function takes as an input a buffer of `size` bytes with an IPP frame (this
is probably a payload of a HTTP request), tries to parse it and returns parsed
content as a `Frame` object.
Errors detected by the parser are saved to `log` which implements the following
interface:

```c++
class ParserLog {
  /* ... */
  ParserLog(const ParserLog&) = delete;
  ParserLog& operator=(const ParserLog&) = delete;
  /* ... */

  // Reports an `error` when parsing an element pointed by `path`.
  virtual void AddParserError(const AttrPath& path, ParserCode error) = 0;

  // Reports that parsing was finished successfully. Calling it means that the
  // parser reached the marker of the end of the frame and the parsed structure
  // is valid. It is be called even if some minor errors were reported earlier.
  virtual void ReportSuccessfulEndOfParsing() = 0;
};
```

There is a predefined class `SimpleParserLog` that implements this interface and
allows to browse reported errors:

```c++
class SimpleParserLog : public ParserLog {
  /* ... */
  struct Entry {
    AttrPath path;
    ParserCode error;
    /* ... */
  };
  explicit SimpleParserLog(size_t max_entries_count = 100);
  /* ... */

  // Returns all entries added by AddParserError() in the same order they were
  // added. The log is truncated <=> the number of entries reached the value
  // `max_entries_count` passed to the constructor.
  const std::vector<Entry>& Entries() const;

  // Returns true <=> ReportSuccessfulEndOfParsing() was called.
  bool ParserFinishSuccessfully() const;
  ...
};
```

> WARNING: The log is truncated when the number of entries reach the threshold
> specified in the constructor.

## Builder

The header `builder.h` contains the following functions:

```c++
// Return the size of the binary representation of the frame in bytes.
size_t CalculateLengthOfBinaryFrame(const Frame& frame);

// Save the binary representation of the frame to the given buffer. Use
// CalculateLengthOfBinaryFrame() function before calling this method to make
// sure that the given buffer is large enough. The method returns the number of
// bytes written to `buffer` or 0 when `buffer_length` is smaller than binary
// representation of the frame.
size_t BuildBinaryFrame(const Frame& frame, uint8_t* buffer, size_t buffer_length);

// Return the binary representation of the frame as a vector.
std::vector<uint8_t> BuildBinaryFrame(const Frame& frame);
```

Function `BuildBinaryFrame()` returns binary representation of the given frame
that is ready to be sent as a payload in a HTTP frame.

## Validator

Validator checks if the values contained in the given `Frame` object meet the
criteria required by IPP standard.
All frames sent to other parties should pass the validator without producing any
errors to ensure full compatibility.
Errors detected by the validator in incoming frames may be logged for
informational purposes.

To use the validator you have to include the header `validator.h`.
It contains the following declarations:

```c++
// Validates all groups in the `frame`. All detected errors are saved in `log`
// in the order they occur in the original frame. The function returns true <=>
// no errors were detected.
// For string types only the basic features are validated, there is no UTF-8
// parsing or type-specific parsing like URL or MIME types.
bool Validate(const Frame& frame, ErrorsLog& log);
```

The `log` is the abstract interface:

```c++
class ErrorsLog {
 public:
  ErrorsLog(const ErrorsLog&) = delete;
  ErrorsLog& operator=(const ErrorsLog&) = delete;
  /* ... */

  // Reports an `error` for the attribute at `path`. The errors are reported in
  // the same order as they occurred in the frame. Return false if do not want
  // to get any more AddValidationError() calls.
  virtual bool AddValidationError(const AttrPath& path, AttrError error) = 0;
};
```

A simple implementation of this interface is also provided:

```c++
// Simple implementation of the ErrorsLog interface. It just saves the first
// `max_entries_count` (see the constructor) errors in the frame.
class SimpleLog : public ErrorsLog {
 public:
  struct Entry {
    AttrPath path;
    AttrError error;
  };
  explicit SimpleLog(size_t max_entries_count = 100);
  bool AddValidationError(const AttrPath& path, AttrError error) override;
  const std::vector<Entry>& Entries() const;
};
```

## Frame class

The `Frame` class represents an IPP frame.
The instance of this class may be obtained by calling the `Parse` function
described above or by using one of the constructors:

```c++
class Frame {
  /* ... */

  // Constructor. Create an empty frame with all basic parameters set to 0.
  Frame();

  // Constructor. Create a frame and set basic parameters for IPP request.
  // If `set_localization_en_us` is true (default) the Group Operation
  // Attributes is added to the frame with two attributes:
  //   * "attributes-charset"="utf-8"
  //   * "attributes-natural-language"="en-us"
  // If `set_localization_en_us` equals false, you have to add the Group
  // Operation Attributes with these two attributes by hand since they are
  // required to be the first attributes in the frame (see section 4.1.4 from
  // RFC 8011).
  explicit Frame(Operation operation_id,
                 Version version_number = Version::_1_1,
                 int32_t request_id = 1,
                 bool set_localization_en_us = true);

  // Constructor. The same as the previous constructor but for IPP response.
  // There is no differences between frames created with this and the previous
  // constructor. IPP requests and IPP responses have the same structure.
  // Values of operation_id and status_code are saved in the same variable,
  // they are just casted to different enums with static_cast<>(). The parameter
  // `set_localization_en_us_and_status_message` works in similar way as the
  // parameter `set_localization_en_us` in the constructor above but also adds
  // the attribute "status-message" to the Group Operation Attributes. The value
  // of the attribute is set to a string representation of the `status_code`.
  // The attribute "status-message" is defined in the section 4.1.6.2 from
  // RFC 8011.
  explicit Frame(Status status_code,
                 Version version_number = Version::_1_1,
                 int32_t request_id = 1,
                 bool set_localization_en_us_and_status_message = true);

  /* ... */
};
```

### Header

Each IPP frame has three fields in the header:

*   **operation-id** (IPP request) or **status-code** (IPP response) of type
    `int16_t`. The two enums below contain predefined values for these fields:

    ```c++
    enum class Operation : int16_t {
      Print_Job = 0x0002,
      Print_URI = 0x0003,
      Validate_Job = 0x0004,
      Create_Job = 0x0005,
      Send_Document = 0x0006,
      Send_URI = 0x0007,
      Cancel_Job = 0x0008,
      Get_Job_Attributes = 0x0009,
      /* ...
        See "operations-supported" in
        https://www.pwg.org/ipp/ipp-registrations.xml for other possible values.
        ... */
      CUPS_Get_Default = 0x4001,
      CUPS_Get_Printers = 0x4002,
      CUPS_Add_Modify_Printer = 0x4003,
      CUPS_Delete_Printer = 0x4004,
      CUPS_Get_Classes = 0x4005,
      CUPS_Add_Modify_Class = 0x4006,
      CUPS_Delete_Class = 0x4007,
      CUPS_Accept_Jobs = 0x4008,
      /* ...
        See "CUPS IPP Operations" in https://www.cups.org/doc/spec-ipp.html for
        other CUPS-specific values.
        ... */
    };

    enum class Status : int16_t {
      successful_ok = 0x0000,
      successful_ok_ignored_or_substituted_attributes = 0x0001,
      successful_ok_conflicting_attributes = 0x0002,
      successful_ok_ignored_subscriptions = 0x0003,
      successful_ok_too_many_events = 0x0005,
      successful_ok_events_complete = 0x0007,
      client_error_bad_request = 0x0400,
      client_error_forbidden = 0x0401,
      /* ...
        See "Status Codes" in
        https://www.pwg.org/ipp/ipp-registrations.xml for other possible values.
        ... */
    };
    ```

*   **version-number** that contains the version of IPP protocol.
    It is coded as two bytes. The following enum contains predefined values:

    ```c++
    enum class Version : uint16_t {
      _1_0 = 0x0100,
      _1_1 = 0x0101,
      _2_0 = 0x0200,
      _2_1 = 0x0201,
      _2_2 = 0x0202
    };
    ```

*   **request-id** - a field of type `int32_t` that should be used to match an
    IPP response to a corresponding IPP request (which is really not necessary
    for HTTP over TCP). This field is expected to have a positive value.

These three fields can also be changed/read with the following methods:

```c++
class Frame {
  /* ... */

  // Access IPP version number.
  Version& VersionNumber();
  Version VersionNumber() const;

  // Access operation id or status code. OperationId() and StatusCode() refer to
  // the same field, they just cast the same value to different enums. The field
  // is interpreted as operation id in IPP request and as status code in IPP
  // responses.
  uint16_t& OperationIdOrStatusCode();
  Operation OperationId() const;
  Status StatusCode() const;

  // Access request id.
  int32_t& RequestId();
  int32_t RequestId() const;

  /* ... */
};
```

### **attribute-group** and GroupTag

The frame's header is followed by a sequence of **attribute-group**.
Each **attribute-group** is labeled by a one-byte tag that is represented in
*libipp* by the following enum:

```c++
enum class GroupTag : uint8_t {
  operation_attributes = 0x0001,
  job_attributes = 0x0002,
  printer_attributes = 0x0004,
  unsupported_attributes = 0x0005,
  subscription_attributes = 0x0006,
  event_notification_attributes = 0x0007,
  resource_attributes = 0x0008,
  document_attributes = 0x0009,
  system_attributes = 0x000a,
};

// The correct values of GroupTag are 0x01, 0x02, 0x04-0x0f. This function
// checks if given GroupTag is valid.
constexpr bool IsValid(GroupTag tag);

// This array contains all valid GroupTag values and may be used in loops like
//   for (GroupTag gt: kGroupTags) ...
constexpr std::array<GroupTag, 14> kGroupTags{ /* ... */ };
```

In the majority of cases a frame contains at most one **attribute-group** with
given `GroupTag` value.
However, the format of IPP frame allows for more than one group with the same
tag.
Some of the IPP operations use this feature and requires responses containing
a sequence of many **attribute-group** with the same `GroupTag`
(e.g.: [**Get-Jobs**](https://www.rfc-editor.org/rfc/rfc8011#section-4.2.6.2)).

### Accessing **attribute-group**

The following methods can be used to browse **attribute-group** in the `Frame`
object:

```c++
class Frame {
  /* ... */

  // Provides access to groups with the given GroupTag. You can iterate over
  // these groups in the following way:
  //   for (Collection& coll: frame.Groups(GroupTag::job_attributes)) {
  //     ...
  //   }
  // or
  //   for (size_t i = 0; i < frame.Groups(GroupTag::job_attributes).size(); ) {
  //     Collection& coll = frame.Groups(GroupTag::job_attributes)[i++];
  //     ...
  //   }
  // The groups are in the same order as they occur in the frame.
  CollectionsView Groups(GroupTag tag);
  ConstCollectionsView Groups(GroupTag tag) const;

  /* ... */
};

class CollectionsView {
  /* ... */

  CollectionsView(const CollectionsView& cv);
  iterator begin();
  iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;
  size_t size() const;
  bool empty() const;
  Collection& operator[](size_t index);
  const Collection& operator[](size_t index) const;

  /* ... */
};

class ConstCollectionsView {
  /* ... */

  ConstCollectionsView(const ConstCollectionsView& cv);
  explicit ConstCollectionsView(const CollectionsView& cv);
  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;
  size_t size() const;
  bool empty() const;
  const Collection& operator[](size_t index) const;

  /* ... */
};

```

`iterator` and `const_iterator` are bidirectional iterators dereferenceable to
`Collection&` and `const Collection&`, respectively.
Class `Collection` represents a dictionary with attributes and is described in
the next section.

### Adding new **attribute-group**

A new **attribute-group** can be added to the frame with the following methods:

```c++
class Frame {
  /* ... */

  // Add a new group of attributes to the frame. The pointer to the new group
  // (Collection*) is saved in `new_group` if `new_group` != nullptr.
  // The returned value is one of the following:
  //  * Code::kOK
  //  * Code::kInvalidGroupTag
  //  * Code::kTooManyGroups
  // If the returned value != Code::kOK then `new_group` is not modified.
  Code AddGroup(GroupTag tag, Collection** new_group = nullptr);

  /* ... */
};
```

## Collection class

Instances of this class are containers for objects of `Attribute` class that is
described in the next section.
Attributes belonging to the same `Collection` must have unique names.

### Accessing the attributes

Attributes inside the `Collection` can be accessed with the following methods:

```c++
// To iterate over all attributes in the collection use iterators, e.g.:
//
//    for (Attribute& attr: collection) { ... }
//    for (const Attribute& attr: collection) { ... }
//
// Attributes inside the collection are always in the same order they were added
// to it. They will also appear in the same order in the resultant frame.
class Collection {
 public:
  class const_iterator;
  class iterator
  /* ... */

  // Standard methods returning iterators.
  iterator begin();
  iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;

  // Methods return attribute by name. Methods return an iterator end() <=> the
  // collection has no attributes with this name.
  iterator GetAttr(std::string_view name);
  const_iterator GetAttr(std::string_view name) const;

  /* ... */
};
```

`iterator` and `const_iterator` are bidirectional iterators dereferenceable to
`Attribute&` and `const Attribute&`, respectively.

### Adding new attributes

New attributes can be added to the `Collection` object with the following methods:

```c++
class Collection {
  /* ... */

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

  // Add a new attribute with one or more collections. Pointers to created
  // collections are returned in the last parameter. The size of the vector
  // `values` determines the number of collections in the attribute.
  // Possible errors:
  //  * kInvalidName
  //  * kNameConflict
  //  * kValueOutOfRange   (the vector is empty)
  //  * kTooManyAttributes.
  Code AddAttr(const std::string& name, Collection*& value);
  Code AddAttr(const std::string& name, std::vector<Collection*>& values);

  /* ... */
};
```

## Attribute class

Instances of this class can store values defined in IPP specification.

### Tag and name

The type of stored values is defined by one-byte **value-tag** represented by
the following enum:

```c++
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
constexpr bool IsOutOfBand(ValueTag tag);

// Is valid integer type (0x21-0x23).
constexpr bool IsInteger(ValueTag tag);

// Is valid character-string type (0x40-0x5f without 0x4a).
constexpr bool IsString(ValueTag tag);

// Is valid tag.
constexpr bool IsValid(ValueTag tag);
```

Each instance of the `Attribute` class has assigned exactly one tag (`ValueTag`)
and a name (`std::string`).
They are permanent through the whole lifetime of the object and cannot be
changed.
The tag and the name of the attribute can be checked with the following API:

```c++
class Attribute {
  /* ... */

  // Returns tag of the attribute.
  ValueTag Tag() const;

  // Returns an attribute's name. It is always a non-empty string.
  std::string_view Name() const;

  /* ... */
};
```

### Values

Attributes have also a vector of values whose type is determined by the
attribute's `ValueTag`.
Attributes can be divided into three groups by type:

*   **out-of-band** (`IsOutOfBand(tag) == true`) - these attributes
    cannot have any values, they have only name and tag.
*   standard attributes (`IsOutOfBand(tag) == false && tag != collection`) -
    these attributes always have assigned a non-empty vector of values.
    *libipp* provides dedicated types for some specific tags:

    ```c++
    // It is used to hold name and text values (see [rfc8010]).
    // If language == "" it represents nameWithoutLanguage or textWithoutLanguage,
    // otherwise it represents nameWithLanguage or textWithLanguage.
    struct StringWithLanguage {
      std::string value = "";
      std::string language = "";
    };

    // Represents resolution type from [rfc8010].
    struct Resolution {
      int32_t xres = 0;
      int32_t yres = 0;
      enum Units : int8_t {
        kDotsPerInch = 3,
        kDotsPerCentimeter = 4
      } units = kDotsPerInch;
    };

    // Represents rangeOfInteger type from [rfc8010].
    struct RangeOfInteger {
      int32_t min_value = 0;
      int32_t max_value = 0;
    };

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
    ```

*   attributes with collections (`tag == collection`) - these attributes
    contains a non-empty vector of `Collection` objects (with
    sub-attributes).

The `Attribute` class has the following interface allowing to get and set the
number of containing values:

```c++
class Attribute {
  /* ... */

  // Returns the current number of elements (values or Collections).
  // It returns 0 <=> IsOutOfBand(Tag()).
  size_t Size() const;

  // Resizes the attribute (changes the number of stored values/collections).
  // When (IsOutOfBand(Tag()) or `new_size` equals 0 this method does nothing.
  void Resize(size_t new_size);

  /* ... */
};
```
