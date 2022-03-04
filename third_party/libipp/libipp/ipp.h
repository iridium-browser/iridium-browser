// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_H_
#define LIBIPP_IPP_H_

// What is this?
// -------------
// General library for building and parsing IPP frames. IPP stands for Internet
// Printing Protocol and is defined in several documents. This implementation
// is based mainly on the following sources:
// * rfc8010
// * rfc8011
// * CUPS Implementation of IPP at https://www.cups.org/doc/spec-ipp.html
// * IPP registry at https://www.pwg.org/ipp/ipp-registrations.xml
// In general, all non-deprecated IPP Operations from rfc8011 and "CUPS
// Implementation of IPP" are implemented. Vast majority of attributes that can
// be returned by these operations are recognized.
//
// Header files
// ------------
// To use the library include only this header (ipp.h). It includes four other
// header files:
// * ipp_base.h - main types and classes, look there for more details
// * ipp_attribute.h, ipp_package.h - classes defines general frame structure
// * ipp_enums.h - enums corresponding to IPP enums and keywords
// * ipp_operations.h - classes representing IPP requests and responses
// * ipp_collections.h - structs corresponding to IPP attributes being
//   collections, i.e. consisting of other attributes.
//
// General assumptions
// -------------------
// * all entities are defined in ipp namespace
// * many functions/methods returns bool to signal success or error:
//   - true means success
//   - false means error
// * vast majority of defined enums and basic types has corresponding functions
//   converting between their values and corresponding string representations:
//   - std::string ToString(type v) - converts v to std::string based on its
//     type. If given value cannot be converted (e.g unknown enum value) it
//     returns empty string.
//   - bool FromString(const std::string& s, type* v) - parses string and try to
//     convert it to value of type "type". If successful, saves parsed value to
//     the parameter v and returns true. Otherwise, returns false and leave the
//     parameter v untouched.
//
// The most important classes
// --------------------------
// * Client class for building IPP requests and parsing IPP responses
// * Server class for parsing IPP requests and building IPP responses
// * Request_* classes from ipp_operations.h representing IPP requests
// * Response_* classes from ipp_operations.h representing IPP responses
// * Attribute, Collection, Group, Package are base classes with general API to
//   browse structure of IPP frame
//
// Examples
// --------
// * ask printer for printer-make-and-model with Get-Printer-Attributes
//
//   std::string url = "ipp://my.printer/ipp";
//   std::vector<uint8_t> frame, frame2;  // these are buffers for IPP frames
//
//   ipp::Request_Get_Printer_Attributes request;
//   request.operation_attributes.printer_uri.Set(url);
//
//   ipp::Client client;
//   client.BuildRequestFrom(&request);
//   client.WriteRequestFrameTo(frame);
//
//   // ... Now you have to send the content of frame as a payload in HTTP POST
//   // ... request with content-type set to application/ipp.
//   // ... Then you have to retrieve a payload from obtained HTTP response and
//   // ... save it to the buffer frame2. Remember to check for HTTP errors.
//
//   ipp::Response_Get_Printer_Attributes response;
//   if (!client.ReadResponseFrameFrom(frame2)) {
//     // parsing error, check client.GetErrorLog()
//     return false;
//   }
//   if (!client.ParseResponseAndSaveTo(&response)) {
//     // parsing error, check client.GetErrorLog()
//     return false;
//   }
//   auto & attribute = response.operation_attributes.printer_make_and_model;
//   if (attribute.GetState() != ipp::AttrState::set)
//     std::cout << "Not included in the response :(\n";
//   else
//     std::cout << attribute.Get().value << "\n";
//
//
// * use CUPS-Get-Printers to query server for list of printers:
//
//   ipp::Request_CUPS_Get_Printers rq;
//   rq.operation_attributes.requested_attributes.Set(
//       {ipp::E_requested_attributes::printer_description});
//   ipp::Client cl;
//   cl.BuildRequestFrom(&rq);
//   std::vector<uint8_t> frame;
//   cl.WriteRequestFrameTo(frame);
//
//   // ... Now you have to send the content of frame as a payload in HTTP POST
//   // ... request with content-type set to application/ipp.
//   // ... Then you have to retrieve a payload from obtained HTTP response and
//   // ... save it to the buffer frame2. Remember to check for HTTP errors.
//
//   std::vector<uint8_t> frame2; // contains IPP frame with response
//   ipp::Response_CUPS_Get_Printers rs;
//   if (cl.ReadResponseFrameFrom(frame2) && cl.ParseResponseAndSaveTo(&rs)) {
//     // success, but it is good to check error log anyway
//     // there may be some warnings about incorrect fields
//     for (auto & l: cl.GetErrorLog())
//       std::err << "WARNING: " << l.message << "\n";
//     if (rs.status_code == Status::successful_ok) {
//       // iterate over printers and print out print-name
//       for (size_t i = 0; i < rs.printer_attributes.Size(); ++i) {
//         std::cout << rs.printer_attributes[i].printer_name.Get().value;
//         std::cout << std::endl;
//       }
//     } else {
//       std::cerr << "server returned IPP code: ";
//       std::cerr << ipp::ToString(rs.status_code) << std::endl;
//     }
//   } else {
//     // error - cannot parse obtained frame
//     std::err << "ERROR: IPP response is incorrect and cannot be parsed:\n";
//     for (auto & l: cl.GetErrorLog())
//       std::err << " * " << l.message << "\n";
//   }
//
//
// * In ipp_test.cc in functions Test1()-Test9() you can find more examples how
//   to build IPP frame.
//
// Notes to examples
// -----------------
// * The following attributes from group operation-attributes are set
//   automatically, if not specified by hand:
//   - attributes-charset -> utf-8
//   - attributes-natural-language -> en-us
//   - status-message -> ToString(this->status_code) - only for Response.
// * The method GetErrorLog() from Client/Server may return non-empty vector
//   even when all functions returned true. The parser can recover from many
//   local errors in IPP frames and ignore some incorrect values. Information
//   about all these events are logged to this vector.
// * If an attribute didn't appear in a response or is not supposed to be
//   present in a request, his state is set to AttrState::unset. It is default
//   value. Methods Set(...) automatically switch attribute's state to
//   AttrState::set. Attribute's state can be set by hand with method
//   Attribute::SetState(...). Other possible values besides AttrState::set and
//   AttrState::unset are out-of-band values (see [rfc8010] and [rfc8011]).
// * If the parsed frame contains an attribute that is no present in the IPP
//   schema, a general "unknown" attribute is added to the package. Unknown
//   attributes behave as known ones, the only difference is that they do not
//   appear in predefined structs.
//
// Naming conventions
// ------------------
// C++ names of operations, groups, attributes, enums and keywords are copied
// from IPP specification. Characters '-' and '.' were replaced by '_'. Names
// of main entities are constructed as follows:
// * Operations: classes Request_(IPP name) and Response_(IPP name), eg.:
//   - Request_Print_Job -> a request Print-Job
//   - Response_CUPS_Get_Printers -> a response for a request CUPS-Get-Printers
// * Groups: fields defined in Request/Response Operation classes, eg.:
//   - operation_attributes -> operation-attributes
//   - job_attributes -> job-attributes
// * Attributes: fields defined in Groups or inside other Attributes, eg.:
//   - attributes_natural_language -> attributes-natural-language
//   - printer_uri -> printer-uri
// * Enums and Keywords: enums E_(IPP name), eg.:
//   - E_pdf_versions_supported -> keyword pdf-versions-supported
//   - E_finishings -> enum finishings
// * Values of Enums and Keywords: just (IPP name), eg.:
//   - E_pdf_versions_supported::adobe_1_4 -> a value adobe-1.4
//   - E_finishings::staple_top_right -> a value staple-top-right
// For C++ enums corresponding to IPP enums all elements has numerical values
// from IPP specification (may be casted to uint16_t to get assigned IPP value).

// look here for Attribute class
#include "ipp_attribute.h"  // NOLINT(build/include)

// look here for main classes: Client, Server, Request, Response
#include "ipp_base.h"  // NOLINT(build/include)

// these files are generated from IPP schema
#include "ipp_collections.h"  // NOLINT(build/include)
#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_operations.h"  // NOLINT(build/include)

// look here for Package, Group and Collection classes
#include "ipp_package.h"  // NOLINT(build/include)

#endif  //  LIBIPP_IPP_H_
