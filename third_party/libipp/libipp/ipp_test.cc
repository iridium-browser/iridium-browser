// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// This file contains unit test based on examples from [rfc8010].

namespace ipp {

static bool operator==(const ipp::Resolution& a, const ipp::Resolution& b) {
  return (a.size1 == b.size1 && a.size2 == b.size2 && a.units == b.units);
}

static bool operator==(const ipp::RangeOfInteger& a,
                       const ipp::RangeOfInteger& b) {
  return (a.min_value == b.min_value && a.max_value == b.max_value);
}

static bool operator==(const ipp::DateTime& a, const ipp::DateTime& b) {
  return (a.UTC_direction == b.UTC_direction && a.UTC_hours == b.UTC_hours &&
          a.UTC_minutes == b.UTC_minutes && a.day == b.day &&
          a.hour == b.hour && a.deci_seconds == b.deci_seconds &&
          a.minutes == b.minutes && a.month == b.month &&
          a.seconds == b.seconds && a.year == b.year);
}

}  // namespace ipp

namespace {

uint32_t TwosComplementEncoding(int value) {
  if (value >= 0)
    return value;
  uint32_t binary = -static_cast<int64_t>(value);
  binary = ~binary;
  ++binary;
  return binary;
}

// Helps build binary representation of frames from examples copied from
// [rfc8010].
struct BinaryContent {
  // frame content
  std::vector<uint8_t> data;
  // add field with ASCII string
  void s(std::string s) {
    for (auto c : s) {
      data.push_back(static_cast<uint8_t>(c));
    }
  }
  // add 1 byte
  void u1(int v) {
    const uint32_t b = TwosComplementEncoding(v);
    data.push_back(b & 0xffu);
  }
  // add 2 bytes
  void u2(int v) {
    const uint32_t b = TwosComplementEncoding(v);
    data.push_back((b >> 8) & 0xffu);
    data.push_back(b & 0xffu);
  }
  // add 4 bytes
  void u4(int v) {
    const uint32_t b = TwosComplementEncoding(v);
    data.push_back((b >> 24) & 0xffu);
    data.push_back((b >> 16) & 0xffu);
    data.push_back((b >> 8) & 0xffu);
    data.push_back(b & 0xffu);
  }
};

// function comparing if two collections have the same content
void CompareCollections(ipp::Collection* c1, ipp::Collection* c2) {
  std::vector<ipp::Attribute*> a1 = c1->GetAllAttributes();
  std::vector<ipp::Attribute*> a2 = c2->GetAllAttributes();
  ASSERT_EQ(a1.size(), a2.size());
  for (size_t i = 0; i < a1.size(); ++i) {
    EXPECT_EQ(a1[i]->GetName(), a2[i]->GetName());
    EXPECT_EQ(a1[i]->GetType(), a2[i]->GetType());
    EXPECT_EQ(a1[i]->IsASet(), a2[i]->IsASet());
    EXPECT_EQ(a1[i]->GetState(), a2[i]->GetState());
    ASSERT_EQ(a1[i]->GetSize(), a2[i]->GetSize());
    for (size_t j = 0; j < a1[i]->GetSize(); ++j) {
      std::string s1, s2;
      ipp::DateTime d1, d2;
      ipp::Resolution r1, r2;
      ipp::RangeOfInteger i1, i2;
      ipp::StringWithLanguage l1, l2;
      switch (a1[i]->GetType()) {
        case ipp::AttrType::text:
        case ipp::AttrType::name:
          a1[i]->GetValue(&l1, j);
          a2[i]->GetValue(&l2, j);
          EXPECT_EQ(l1.value, l2.value);
          EXPECT_EQ(l1.language, l2.language);
          break;
        case ipp::AttrType::integer:
        case ipp::AttrType::boolean:
        case ipp::AttrType::enum_:
        case ipp::AttrType::octetString:
        case ipp::AttrType::keyword:
        case ipp::AttrType::uri:
        case ipp::AttrType::uriScheme:
        case ipp::AttrType::charset:
        case ipp::AttrType::naturalLanguage:
        case ipp::AttrType::mimeMediaType:
          a1[i]->GetValue(&s1, j);
          a2[i]->GetValue(&s2, j);
          EXPECT_EQ(s1, s2);
          break;
        case ipp::AttrType::dateTime:
          a1[i]->GetValue(&d1, j);
          a2[i]->GetValue(&d2, j);
          EXPECT_EQ(d1, d2);
          break;
        case ipp::AttrType::resolution:
          a1[i]->GetValue(&r1, j);
          a2[i]->GetValue(&r2, j);
          EXPECT_EQ(r1, r2);
          break;
        case ipp::AttrType::rangeOfInteger:
          a1[i]->GetValue(&i1, j);
          a2[i]->GetValue(&i2, j);
          EXPECT_EQ(i1, i2);
          break;
        case ipp::AttrType::collection:
          CompareCollections(a1[i]->GetCollection(j), a2[i]->GetCollection(j));
          break;
      }
    }
  }
}

// checks if given frame is a binary representation of given Request
void CheckRequest(const BinaryContent& frame,
                  ipp::Request* req,
                  const int32_t request_no = 1) {
  // build output frame from Request and compare with the given frame
  ipp::Client client(ipp::Version::_1_1, request_no - 1);
  client.BuildRequestFrom(req);
  EXPECT_TRUE(client.GetErrorLog().empty());
  std::vector<uint8_t> bin_data;
  ASSERT_TRUE(client.WriteRequestFrameTo(&bin_data));
  EXPECT_TRUE(client.GetErrorLog().empty());
  EXPECT_EQ(client.GetFrameLength(), bin_data.size());
  EXPECT_EQ(bin_data, frame.data);
  // parse the given frame and compare obtained object with the given Request
  ipp::Server server(ipp::Version::_1_1, request_no);
  auto req2 = ipp::Request::NewRequest(req->GetOperationId());
  ASSERT_TRUE(server.ReadRequestFrameFrom(bin_data));
  EXPECT_TRUE(server.GetErrorLog().empty());
  ASSERT_TRUE(server.ParseRequestAndSaveTo(req2.get()));
  EXPECT_TRUE(server.GetErrorLog().empty());
  // ... compares two request objects
  std::vector<ipp::Group*> groups1 = req->GetAllGroups();
  std::vector<ipp::Group*> groups2 = req2->GetAllGroups();
  ASSERT_EQ(groups1.size(), groups2.size());
  for (size_t i = 0; i < groups1.size(); ++i) {
    ipp::Group* g1 = groups1[i];
    ipp::Group* g2 = groups2[i];
    EXPECT_EQ(g1->GetName(), g2->GetName());
    ASSERT_EQ(g1->GetSize(), g2->GetSize());
    for (size_t j = 0; j < g1->GetSize(); ++j)
      CompareCollections(g1->GetCollection(j), g2->GetCollection(j));
  }
  EXPECT_EQ(req->Data(), req2->Data());
}

// checks if given frame is a binary representation of given Response
void CheckResponse(const BinaryContent& frame,
                   ipp::Response* res,
                   const int32_t request_no = 1) {
  // build output frame from Response and compare with the given frame
  ipp::Server server(ipp::Version::_1_1, request_no);
  server.BuildResponseFrom(res);
  EXPECT_TRUE(server.GetErrorLog().empty());
  std::vector<uint8_t> bin_data;
  ASSERT_TRUE(server.WriteResponseFrameTo(&bin_data));
  EXPECT_TRUE(server.GetErrorLog().empty());
  EXPECT_EQ(server.GetFrameLength(), bin_data.size());
  EXPECT_EQ(bin_data, frame.data);
  // parse the given frame and compare obtained object with the given Response
  ipp::Client client(ipp::Version::_1_1, request_no - 1);
  auto res2 = ipp::Response::NewResponse(res->GetOperationId());
  ASSERT_TRUE(client.ReadResponseFrameFrom(bin_data));
  EXPECT_TRUE(client.GetErrorLog().empty());
  ASSERT_TRUE(client.ParseResponseAndSaveTo(res2.get()));
  EXPECT_TRUE(client.GetErrorLog().empty());
  // ... compares two response objects
  EXPECT_EQ(res->StatusCode(), res2->StatusCode());
  std::vector<ipp::Group*> groups1 = res->GetAllGroups();
  std::vector<ipp::Group*> groups2 = res2->GetAllGroups();
  ASSERT_EQ(groups1.size(), groups2.size());
  for (size_t i = 0; i < groups1.size(); ++i) {
    ipp::Group* g1 = groups1[i];
    ipp::Group* g2 = groups2[i];
    EXPECT_EQ(g1->GetName(), g2->GetName());
    ASSERT_EQ(g1->GetSize(), g2->GetSize());
    for (size_t j = 0; j < g1->GetSize(); ++j)
      CompareCollections(g1->GetCollection(j), g2->GetCollection(j));
  }
  EXPECT_EQ(res->Data(), res2->Data());
}

TEST(rfc8010, example1) {
  // A.1.  Print-Job Request
  // The following is an example of a Print-Job request with "job-name",
  // "copies", and "sides" specified.  The "ipp-attribute-fidelity"
  // attribute is set to 'true' so that the print request will fail if the
  // "copies" or the "sides" attribute is not supported or their values
  // are not supported.
  BinaryContent c;
  // Octets                                Symbolic Value       Protocol field
  c.u2(0x0101u);                          // 1.1                  version-number
  c.u2(0x0002u);                          // Print-Job            operation-id
  c.u4(0x00000001u);                      // 1                    request-id
  c.u1(0x01u);                            // start operation-     operation-
                                          // attributes           attributes-tag
  c.u1(0x47u);                            // charset type         value-tag
  c.u2(0x0012u);                          // name-length
  c.s("attributes-charset");              // attributes-charset   name
  c.u2(0x0005u);                          // value-length
  c.s("utf-8");                           // UTF-8                value
  c.u1(0x48u);                            // natural-language     value-tag
                                          // type
  c.u2(0x001bu);                          // name-length
  c.s("attributes-natural-language");     // attributes-natural-  name
                                          // language
  c.u2(0x0005u);                          // value-length
  c.s("en-us");                           // en-US                value
  c.u1(0x45u);                            // uri type             value-tag
  c.u2(0x000bu);                          // name-length
  c.s("printer-uri");                     // printer-uri          name
  c.u2(0x002cu);                          // value-length
  c.s("ipp://printer.example.com/ipp/");  // printer pinetree     value
  c.s("print/pinetree");
  c.u1(0x42u);                    // nameWithoutLanguage  value-tag
                                  // type
  c.u2(0x0008u);                  // name-length
  c.s("job-name");                // job-name             name
  c.u2(0x0006u);                  // value-length
  c.s("foobar");                  // foobar               value
  c.u1(0x22u);                    // boolean type         value-tag
  c.u2(0x0016u);                  // name-length
  c.s("ipp-attribute-fidelity");  // ipp-attribute-       name
                                  // fidelity
  c.u2(0x0001u);                  // value-length
  c.u1(0x01u);                    // true                 value
  c.u1(0x02u);                    // start job-attributes job-attributes-
                                  // tag
  c.u1(0x21u);                    // integer type         value-tag
  c.u2(0x0006u);                  // name-length
  c.s("copies");                  // copies               name
  c.u2(0x0004u);                  // value-length
  c.u4(0x00000014u);              // 20                   value
  c.u1(0x44u);                    // keyword type         value-tag
  c.u2(0x0005u);                  // name-length
  c.s("sides");                   // sides                name
  c.u2(0x0013u);                  // value-length
  c.s("two-sided-long-edge");     // two-sided-long-edge  value
  c.u1(0x03u);                    // end-of-attributes    end-of-
                                  // attributes-tag
  c.s("%!PDF...");                // <PDF Document>       data

  ipp::Request_Print_Job r;
  r.operation_attributes.printer_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree");
  r.operation_attributes.job_name.Set("foobar");
  r.operation_attributes.ipp_attribute_fidelity.Set(1);
  r.job_attributes.copies.Set(20);
  r.job_attributes.sides.Set(ipp::E_sides::two_sided_long_edge);
  for (char c : std::string("%!PDF..."))
    r.Data().push_back(static_cast<uint8_t>(c));

  CheckRequest(c, &r);
}

TEST(rfc8010, example2) {
  // A.2.  Print-Job Response (Successful)
  // Here is an example of a successful Print-Job response to the previous
  // Print-Job request.  The Printer supported the "copies" and "sides"
  // attributes and their supplied values.  The status-code returned is
  // 'successful-ok'.
  BinaryContent c;
  // Octets                                  Symbolic Value     Protocol field
  c.u2(0x0101u);                       // 1.1                version-number
  c.u2(0x0000u);                       // successful-ok      status-code
  c.u4(0x00000001u);                   // 1                  request-id
  c.u1(0x01u);                         // start operation-   operation-
                                       // attributes         attributes-tag
  c.u1(0x47u);                         // charset type       value-tag
  c.u2(0x0012u);                       // name-length
  c.s("attributes-charset");           // attributes-charset name
  c.u2(0x0005u);                       // value-length
  c.s("utf-8");                        // UTF-8              value
  c.u1(0x48u);                         // natural-language   value-tag
                                       // type
  c.u2(0x001bu);                       // name-length
  c.s("attributes-natural-language");  // attributes-        name
                                       // natural-language
  c.u2(0x0005u);                       // value-length
  c.s("en-us");                        // en-US              value
  c.u1(0x41u);                         // textWithoutLanguag value-tag
                                       // e type
  c.u2(0x000eu);                       // name-length
  c.s("status-message");               // status-message     name
  c.u2(0x000du);                       // value-length
  c.s("successful-ok");                // successful-ok      value
  c.u1(0x02u);                         // start job-         job-attributes-
                                       // attributes         tag
  c.u1(0x21u);                         // integer            value-tag
  c.u2(0x0006u);                       // name-length
  c.s("job-id");                       // job-id             name
  c.u2(0x0004u);                       // value-length
  c.u4(147);                           // 147                value
  c.u1(0x45u);                         // uri type           value-tag
  c.u2(0x0007u);                       // name-length
  c.s("job-uri");                      // job-uri            name
  c.u2(0x0030u);                       // value-length
  c.s("ipp://printer.example.com/ipp/pr");  // job 147 on         value
  c.s("int/pinetree/147");                  // pinetree
  c.u1(0x23u);                              // enum type          value-tag
  c.u2(0x0009u);                            // name-length
  c.s("job-state");                         // job-state          name
  c.u2(0x0004u);                            // value-length
  c.u4(0x0003u);                            // pending            value
  c.u1(0x03u);                              // end-of-attributes  end-of-
                                            // attributes-tag

  ipp::Response_Print_Job r;
  r.job_attributes.job_id.Set(147);
  r.job_attributes.job_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree/147");
  r.job_attributes.job_state.Set(ipp::E_job_state::pending);

  CheckResponse(c, &r);
}

TEST(rfc8010, example3) {
  // A.3.  Print-Job Response (Failure)
  // Here is an example of an unsuccessful Print-Job response to the
  // previous Print-Job request.  It fails because, in this case, the
  // Printer does not support the "sides" attribute and because the value
  // '20' for the "copies" attribute is not supported.  Therefore, no Job
  // is created, and neither a "job-id" nor a "job-uri" operation
  // attribute is returned.  The error code returned is 'client-error-
  // attributes-or-values-not-supported' (0x040b).
  BinaryContent c;
  // Octets                            Symbolic Value              Protocol
  // field
  c.u2(0x0101u);              // 1.1                         version-
                              // number
  c.u2(0x040bu);              // client-error-attributes-or- status-code
                              // values-not-supported
  c.u4(0x00000001u);          // 1                           request-id
  c.u1(0x01u);                // start operation-attributes  operation-
                              // attributes
                              // tag
  c.u1(0x47u);                // charset type               value-tag
  c.u2(0x0012u);              // name-length
  c.s("attributes-charset");  // attributes-charset         name
  c.u2(0x0005u);              // value-length
  c.s("utf-8");               // UTF-8                      value
  c.u1(0x48u);                // natural-language type      value-tag
  c.u2(0x001bu);              // name-length
  c.s("attributes-natural-language");  // attributes-natural-language name
  c.u2(0x0005u);                       // value-length
  c.s("en-us");                        // en-US                      value
  c.u1(0x41u);                         // textWithoutLanguage type   value-tag
  c.u2(0x000eu);                       // name-length
  c.s("status-message");               // status-message             name
  c.u2(0x002fu);                       // value-length
  c.s("client-error-attributes-or-");  // client-error-attributes-or- value
  c.s("values-not-supported");         // values-not-supported
  c.u1(0x05u);        // start unsupported-         unsupported-
                      // attributes                 attributes
                      // tag
  c.u1(0x21u);        // integer type               value-tag
  c.u2(0x0006u);      // name-length
  c.s("copies");      // copies                     name
  c.u2(0x0004u);      // value-length
  c.u4(0x00000014u);  // 20                         value
  c.u1(0x10u);        // unsupported (type)         value-tag
  c.u2(0x0005u);      // name-length
  c.s("sides");       // sides                      name
  c.u2(0x0000u);      // value-length
  c.u1(0x03u);        // end-of-attributes          end-of-
                      // attributes-
                      // tag

  ipp::Response_Print_Job r;
  r.StatusCode() =
      ipp::E_status_code::client_error_attributes_or_values_not_supported;
  ipp::Attribute* a = r.unsupported_attributes.AddUnknownAttribute(
      "copies", true, ipp::AttrType::integer);
  a->SetValue(20);
  a = r.unsupported_attributes.AddUnknownAttribute("sides", true,
                                                   ipp::AttrType::integer);
  a->SetState(ipp::AttrState::unsupported);

  CheckResponse(c, &r);
}

TEST(rfc8010, example4) {
  // A.4.  Print-Job Response (Success with Attributes Ignored)
  // Here is an example of a successful Print-Job response to a Print-Job
  // request like the previous Print-Job request, except that the value of
  // "ipp-attribute-fidelity" is 'false'.  The print request succeeds,
  // even though, in this case, the Printer supports neither the "sides"
  // attribute nor the value '20' for the "copies" attribute.  Therefore,
  // a Job is created and both a "job-id" and a "job-uri" operation
  // attribute are returned.  The unsupported attributes are also returned
  // in an Unsupported Attributes group.  The error code returned is
  // 'successful-ok-ignored-or-substituted-attributes' (0x0001).
  BinaryContent c;
  // Octets                            Symbolic Value              Protocol
  // field
  c.u2(0x0101u);  // 1.1                         version-number
  c.u2(0x0001u);  // successful-ok-ignored-or-   status-coder.StatusCode() =
                  // static_cast<uint16_t>(ipp::E_status_code::successful_ok);
                  // substituted-attributes
  c.u4(0x00000001u);           // 1                           request-id
  c.u1(0x01u);                 // start operation-attributes  operation-
                               // attributes-tag
  c.u1(0x47u);                 // charset type                value-tag
  c.u2(0x0012u);               // name-length
  c.s("attributes-charset");   // attributes-charset          name
  c.u2(0x0005u);               // value-length
  c.s("utf-8");                // UTF-8                       value
  c.u1(0x48u);                 // natural-language type       value-tag
  c.u2(0x001bu);               // name-length
  c.s("attributes-natural-");  // attributes-natural-language name
  c.s("language");
  c.u2(0x0005u);                     // value-length
  c.s("en-us");                      // en-US                       value
  c.u1(0x41u);                       // textWithoutLanguage type    value-tag
  c.u2(0x000eu);                     // name-length
  c.s("status-message");             // status-message              name
  c.u2(0x002fu);                     // value-length
  c.s("successful-ok-ignored-or-");  // successful-ok-ignored-or-   value
  c.s("substituted-attributes");     // substituted-attributes
  c.u1(0x05u);                       // start unsupported-          unsupported-
                // attributes                  attributes tag
  c.u1(0x21u);                        // integer type                value-tag
  c.u2(0x0006u);                      // name-length
  c.s("copies");                      // copies                      name
  c.u2(0x0004u);                      // value-length
  c.u4(0x00000014u);                  // 20                          value
  c.u1(0x10u);                        // unsupported  (type)         value-tag
  c.u2(0x0005u);                      // name-length
  c.s("sides");                       // sides                       name
  c.u2(0x0000u);                      // value-length
  c.u1(0x02u);                        // start job-attributes        job-
                                      // attributes-tag
  c.u1(0x21u);                        // integer                     value-tag
  c.u2(0x0006u);                      // name-length
  c.s("job-id");                      // job-id                      name
  c.u2(0x0004u);                      // value-length
  c.u4(147);                          // 147                         value
  c.u1(0x45u);                        // uri type                    value-tag
  c.u2(0x0007u);                      // name-length
  c.s("job-uri");                     // job-uri                     name
  c.u2(0x0030u);                      // value-length
  c.s("ipp://printer.example.com/");  // job 147 on pinetree         value
  c.s("ipp/print/pinetree/147");
  c.u1(0x23u);       // enum  type                  value-tag
  c.u2(0x0009u);     // name-length
  c.s("job-state");  // job-state                   name
  c.u2(0x0004u);     // value-length
  c.u4(0x0003u);     // pending                     value
  c.u1(0x03u);       // end-of-attributes           end-of-
                     // attributes-tag

  ipp::Response_Print_Job r;
  r.StatusCode() =
      ipp::E_status_code::successful_ok_ignored_or_substituted_attributes;
  ipp::Attribute* a = r.unsupported_attributes.AddUnknownAttribute(
      "copies", true, ipp::AttrType::integer);
  a->SetValue(20);
  a = r.unsupported_attributes.AddUnknownAttribute("sides", true,
                                                   ipp::AttrType::integer);
  a->SetState(ipp::AttrState::unsupported);
  r.job_attributes.job_id.Set(147);
  r.job_attributes.job_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree/147");
  r.job_attributes.job_state.Set(ipp::E_job_state::pending);

  CheckResponse(c, &r);
}

TEST(rfc8010, example5) {
  // A.5.  Print-URI Request
  // The following is an example of Print-URI request with "copies" and
  // "job-name" parameters:
  BinaryContent c;
  // Octets                                Symbolic Value       Protocol field
  c.u2(0x0101u);                          // 1.1                  version-number
  c.u2(0x0003u);                          // Print-URI            operation-id
  c.u4(0x00000001u);                      // 1                    request-id
  c.u1(0x01u);                            // start operation-     operation-
                                          // attributes           attributes-tag
  c.u1(0x47u);                            // charset type         value-tag
  c.u2(0x0012u);                          // name-length
  c.s("attributes-charset");              // attributes-charset   name
  c.u2(0x0005u);                          // value-length
  c.s("utf-8");                           // UTF-8                value
  c.u1(0x48u);                            // natural-language     value-tag
                                          // type
  c.u2(0x001bu);                          // name-length
  c.s("attributes-natural-language");     // attributes-natural-  name
                                          // language
  c.u2(0x0005u);                          // value-length
  c.s("en-us");                           // en-US                value
  c.u1(0x45u);                            // uri type             value-tag
  c.u2(0x000bu);                          // name-length
  c.s("printer-uri");                     // printer-uri          name
  c.u2(0x002cu);                          // value-length
  c.s("ipp://printer.example.com/ipp/");  // printer pinetree     value
  c.s("print/pinetree");
  c.u1(0x45u);                       // uri type             value-tag
  c.u2(0x000cu);                     // name-length
  c.s("document-uri");               // document-uri         name
  c.u2(0x0019u);                     // value-length
  c.s("ftp://foo.example.com/foo");  // ftp://foo.example.co value
                                     // m/foo
  c.u1(0x42u);                       // nameWithoutLanguage  value-tag
                                     // type
  c.u2(0x0008u);                     // name-length
  c.s("job-name");                   // job-name             name
  c.u2(0x0006u);                     // value-length
  c.s("foobar");                     // foobar               value
  c.u1(0x02u);                       // start job-attributes job-attributes-
                                     // tag
  c.u1(0x21u);                       // integer type         value-tag
  c.u2(0x0006u);                     // name-length
  c.s("copies");                     // copies               name
  c.u2(0x0004u);                     // value-length
  c.u4(0x00000001u);                 // 1                    value
  c.u1(0x03u);                       // end-of-attributes    end-of-
                                     // attributes-tag

  ipp::Request_Print_URI r;
  r.operation_attributes.printer_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree");
  r.operation_attributes.document_uri.Set("ftp://foo.example.com/foo");
  r.operation_attributes.job_name.Set("foobar");
  r.job_attributes.copies.Set(1);

  CheckRequest(c, &r);
}

TEST(rfc8010, example6) {
  // A.6.  Create-Job Request
  // The following is an example of Create-Job request with no parameters
  // and no attributes:
  BinaryContent c;
  // Octets                                Symbolic Value       Protocol field
  c.u2(0x0101u);                          // 1.1                  version-number
  c.u2(0x0005u);                          // Create-Job           operation-id
  c.u4(0x00000001u);                      // 1                    request-id
  c.u1(0x01u);                            // start operation-     operation-
                                          // attributes           attributes-tag
  c.u1(0x47u);                            // charset type         value-tag
  c.u2(0x0012u);                          // name-length
  c.s("attributes-charset");              // attributes-charset   name
  c.u2(0x0005u);                          // value-length
  c.s("utf-8");                           // UTF-8                value
  c.u1(0x48u);                            // natural-language     value-tag
                                          // type
  c.u2(0x001bu);                          // name-length
  c.s("attributes-natural-language");     // attributes-natural-  name
                                          // language
  c.u2(0x0005u);                          // value-length
  c.s("en-us");                           // en-US                value
  c.u1(0x45u);                            // uri type             value-tag
  c.u2(0x000bu);                          // name-length
  c.s("printer-uri");                     // printer-uri          name
  c.u2(0x002cu);                          // value-length
  c.s("ipp://printer.example.com/ipp/");  // printer pinetree     value
  c.s("print/pinetree");
  c.u1(0x03u);  // end-of-attributes    end-of-
                // attributes-tag

  ipp::Request_Create_Job r;
  r.operation_attributes.printer_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree");

  CheckRequest(c, &r);
}

TEST(rfc8010, example7) {
  // A.7.  Create-Job Request with Collection Attributes
  // The following is an example of Create-Job request with the "media-
  // col" collection attribute [PWG5100.3] with the value "media-
  // size={x-dimension=21000 y-dimension=29700} media-type='stationery'":
  BinaryContent c;
  // Octets                                Symbolic Value       Protocol field
  c.u2(0x0101u);                          // 1.1                  version-number
  c.u2(0x0005u);                          // Create-Job           operation-id
  c.u4(0x00000001u);                      // 1                    request-id
  c.u1(0x01u);                            // start operation-     operation-
                                          // attributes           attributes-tag
  c.u1(0x47u);                            // charset type         value-tag
  c.u2(0x0012u);                          // name-length
  c.s("attributes-charset");              // attributes-charset   name
  c.u2(0x0005u);                          // value-length
  c.s("utf-8");                           // UTF-8                value
  c.u1(0x48u);                            // natural-language     value-tag
                                          // type
  c.u2(0x001bu);                          // name-length
  c.s("attributes-natural-language");     // attributes-natural-  name
                                          // language
  c.u2(0x0005u);                          // value-length
  c.s("en-us");                           // en-US                value
  c.u1(0x45u);                            // uri type             value-tag
  c.u2(0x000bu);                          // name-length
  c.s("printer-uri");                     // printer-uri          name
  c.u2(0x002cu);                          // value-length
  c.s("ipp://printer.example.com/ipp/");  // printer pinetree     value
  c.s("print/pinetree");
  c.u1(static_cast<int>(ipp::GroupTag::job_attributes));  // missing byte ?
  c.u1(0x34u);         // begCollection        value-tag
  c.u2(0x0009u);       // 9                    name-length
  c.s("media-col");    // media-col            name
  c.u2(0x0000u);       // 0                    value-length
  c.u1(0x4au);         // memberAttrName       value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x000au);       // 10                   value-length
  c.s("media-size");   // media-size           value (member-
                       // name)
  c.u1(0x34u);         // begCollection        member-value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x0000u);       // 0                    member-value-
                       // length
  c.u1(0x4au);         // memberAttrName       value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x000bu);       // 11                   value-length
  c.s("x-dimension");  // x-dimension          value (member-
                       // name)
  c.u1(0x21u);         // integer              member-value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x0004u);       // 4                    member-value-
                       // length
  c.u4(0x00005208u);   // 21000                member-value
  c.u1(0x4au);         // memberAttrName       value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x000bu);       // 11                   value-length
  c.s("y-dimension");  // y-dimension          value (member-
                       // name)
  c.u1(0x21u);         // integer              member-value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x0004u);       // 4                    member-value-
                       // length
  c.u4(0x00007404u);   // 29700                member-value
  c.u1(0x37u);         // endCollection        end-value-tag
  c.u2(0x0000u);       // 0                    end-name-length
  c.u2(0x0000u);       // 0                    end-value-length
  c.u1(0x4au);         // memberAttrName       value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x000au);       // 10                   value-length
  c.s("media-type");   // media-type           value (member-
                       // name)
  c.u1(0x44u);         // keyword              member-value-tag
  c.u2(0x0000u);       // 0                    name-length
  c.u2(0x000au);       // 10                   member-value-
                       // length
  c.s("stationery");   // stationery           member-value
  c.u1(0x37u);         // endCollection        end-value-tag
  c.u2(0x0000u);       // 0                    end-name-length
  c.u2(0x0000u);       // 0                    end-value-length
  c.u1(0x03u);         // end-of-attributes    end-of-
                       // attributes-tag

  ipp::Request_Create_Job r;
  r.operation_attributes.printer_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree");
  r.job_attributes.media_col.media_size.x_dimension.Set(21000);
  r.job_attributes.media_col.media_size.y_dimension.Set(29700);
  r.job_attributes.media_col.media_type.Set(ipp::E_media_type::stationery);

  CheckRequest(c, &r);
}

TEST(rfc8010, example8) {
  // A.8.  Get-Jobs Request
  // The following is an example of Get-Jobs request with parameters but
  // no attributes:
  BinaryContent c;
  // Octets                                Symbolic Value       Protocol field
  c.u2(0x0101u);                          // 1.1                  version-number
  c.u2(0x000au);                          // Get-Jobs             operation-id
  c.u4(0x0000007bu);                      // 123                  request-id
  c.u1(0x01u);                            // start operation-     operation-
                                          // attributes           attributes-tag
  c.u1(0x47u);                            // charset type         value-tag
  c.u2(0x0012u);                          // name-length
  c.s("attributes-charset");              // attributes-charset   name
  c.u2(0x0005u);                          // value-length
  c.s("utf-8");                           // UTF-8                value
  c.u1(0x48u);                            // natural-language     value-tag
                                          // type
  c.u2(0x001bu);                          // name-length
  c.s("attributes-natural-language");     // attributes-natural-  name
                                          // language
  c.u2(0x0005u);                          // value-length
  c.s("en-us");                           // en-US                value
  c.u1(0x45u);                            // uri type             value-tag
  c.u2(0x000bu);                          // name-length
  c.s("printer-uri");                     // printer-uri          name
  c.u2(0x002cu);                          // value-length
  c.s("ipp://printer.example.com/ipp/");  // printer pinetree     value
  c.s("print/pinetree");
  c.u1(0x21u);                  // integer type         value-tag
  c.u2(0x0005u);                // name-length
  c.s("limit");                 // limit                name
  c.u2(0x0004u);                // value-length
  c.u4(0x00000032u);            // 50                   value
  c.u1(0x44u);                  // keyword type         value-tag
  c.u2(0x0014u);                // name-length
  c.s("requested-attributes");  // requested-attributes name
  c.u2(0x0006u);                // value-length
  c.s("job-id");                // job-id               value
  c.u1(0x44u);                  // keyword type         value-tag
  c.u2(0x0000u);                // additional value     name-length
  c.u2(0x0008u);                // value-length
  c.s("job-name");              // job-name             value
  c.u1(0x44u);                  // keyword type         value-tag
  c.u2(0x0000u);                // additional value     name-length
  c.u2(0x000fu);                // value-length
  c.s("document-format");       // document-format      value
  c.u1(0x03u);                  // end-of-attributes    end-of-
                                // attributes-tag

  ipp::Request_Get_Jobs r;
  r.operation_attributes.printer_uri.Set(
      "ipp://printer.example.com/ipp/print/pinetree");
  r.operation_attributes.limit.Set(50);
  r.operation_attributes.requested_attributes.Set(
      {"job-id", "job-name", "document-format"});

  CheckRequest(c, &r, 123);
}

TEST(rfc8010, example9) {
  // A.9.  Get-Jobs Response
  // The following is an example of a Get-Jobs response from a previous
  // request with three Jobs.  The Printer returns no information about
  // the second Job (because of security reasons):
  BinaryContent c;
  // Octets                         Symbolic Value          Protocol field
  c.u2(0x0101u);               // 1.1                     version-number
  c.u2(0x0000u);               // successful-ok           status-code
  c.u4(0x0000007bu);           // 123                     request-id (echoed
                               // back)
  c.u1(0x01u);                 // start operation-        operation-attributes-
                               // attributes              tag
  c.u1(0x47u);                 // charset type            value-tag
  c.u2(0x0012u);               // name-length
  c.s("attributes-charset");   // attributes-charset      name
  c.u2(0x0005u);               // value-length
  c.s("utf-8");                // UTF-8                   value
  c.u1(0x48u);                 // natural-language type   value-tag
  c.u2(0x001bu);               // name-length
  c.s("attributes-natural-");  // attributes-natural-     name
  c.s("language");             // language
  c.u2(0x0005u);               // value-length
  c.s("en-us");                // en-US                   value
  c.u1(0x41u);                 // textWithoutLanguage     value-tag
                               // type
  c.u2(0x000eu);               // name-length
  c.s("status-message");       // status-message          name
  c.u2(0x000du);               // value-length
  c.s("successful-ok");        // successful-ok           value
  c.u1(0x02u);                 // start job-attributes    job-attributes-tag
                               // (1st  object)
  c.u1(0x21u);                 // integer type            value-tag
  c.u2(0x0006u);               // name-length
  c.s("job-id");               // job-id                  name
  c.u2(0x0004u);               // value-length
  c.u4(147);                   // 147                     value
  c.u1(0x36u);                 // nameWithLanguage        value-tag
  c.u2(0x0008u);               // name-length
  c.s("job-name");             // job-name                name
  c.u2(0x000cu);               // value-length
  c.u2(0x0005u);               // sub-value-length
  c.s("fr-ca");                // fr-CA                   value
  c.u2(0x0003u);               // sub-value-length
  c.s("fou");                  // fou                     name
  c.u1(0x02u);                 // start job-attributes    job-attributes-tag
                               // (2nd object)
  c.u1(0x02u);                 // start job-attributes    job-attributes-tag
                               // (3rd object)
  c.u1(0x21u);                 // integer type            value-tag
  c.u2(0x0006u);               // name-length
  c.s("job-id");               // job-id                  name
  c.u2(0x0004u);               // value-length
  c.u4(149);                   // 149                     value
  c.u1(0x36u);                 // nameWithLanguage        value-tag
  c.u2(0x0008u);               // name-length
  c.s("job-name");             // job-name                name
  c.u2(0x0012u);               // value-length
  c.u2(0x0005u);               // sub-value-length
  c.s("de-CH");                // de-CH                   value
  c.u2(0x0009u);               // sub-value-length
  c.s("isch guet");            // isch guet               name
  c.u1(0x03u);                 // end-of-attributes       end-of-attributes-tag

  ipp::Response_Get_Jobs r;
  r.job_attributes[0].job_id.Set(147);
  r.job_attributes[0].job_name.Set(ipp::StringWithLanguage("fou", "fr-ca"));
  r.job_attributes[2].job_id.Set(149);
  r.job_attributes[2].job_name.Set(
      ipp::StringWithLanguage("isch guet", "de-CH"));

  CheckResponse(c, &r, 123);
}

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
