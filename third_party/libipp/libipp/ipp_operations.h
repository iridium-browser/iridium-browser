// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_OPERATIONS_H_
#define LIBIPP_IPP_OPERATIONS_H_

#include <string>
#include <vector>

#include "ipp_attribute.h"  // NOLINT(build/include)
#include "ipp_base.h"  // NOLINT(build/include)
#include "ipp_collections.h"  // NOLINT(build/include)
#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_export.h"  // NOLINT(build/include)
#include "ipp_package.h"  // NOLINT(build/include)

// This file contains definition of classes corresponding to supported IPP
// operations. See ipp.h for more details.
// This file was generated from IPP schema based on the following sources:
// * [rfc8011]
// * [CUPS Implementation of IPP] at https://www.cups.org/doc/spec-ipp.html
// * [IPP registry] at https://www.pwg.org/ipp/ipp-registrations.xml

namespace ipp {
struct IPP_EXPORT Request_CUPS_Add_Modify_Class : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    SetOfValues<E_auth_info_required> auth_info_required{
        AttrName::auth_info_required, AttrType::keyword};
    SetOfValues<std::string> member_uris{AttrName::member_uris, AttrType::uri};
    SingleValue<bool> printer_is_accepting_jobs{
        AttrName::printer_is_accepting_jobs, AttrType::boolean};
    SingleValue<StringWithLanguage> printer_info{AttrName::printer_info,
                                                 AttrType::text};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<std::string> printer_more_info{AttrName::printer_more_info,
                                               AttrType::uri};
    SingleValue<E_printer_state> printer_state{AttrName::printer_state,
                                               AttrType::enum_};
    SingleValue<StringWithLanguage> printer_state_message{
        AttrName::printer_state_message, AttrType::text};
    SetOfValues<StringWithLanguage> requesting_user_name_allowed{
        AttrName::requesting_user_name_allowed, AttrType::name};
    SetOfValues<StringWithLanguage> requesting_user_name_denied{
        AttrName::requesting_user_name_denied, AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Request_CUPS_Add_Modify_Class();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Add_Modify_Class : public Response {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<StringWithLanguage> status_message{AttrName::status_message,
                                                   AttrType::text};
    SingleValue<StringWithLanguage> detailed_status_message{
        AttrName::detailed_status_message, AttrType::text};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Add_Modify_Class();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Add_Modify_Printer : public Request {
  typedef Request_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    SetOfValues<E_auth_info_required> auth_info_required{
        AttrName::auth_info_required, AttrType::keyword};
    SingleValue<E_job_sheets_default> job_sheets_default{
        AttrName::job_sheets_default, AttrType::keyword};
    SingleValue<std::string> device_uri{AttrName::device_uri, AttrType::uri};
    SingleValue<bool> printer_is_accepting_jobs{
        AttrName::printer_is_accepting_jobs, AttrType::boolean};
    SingleValue<StringWithLanguage> printer_info{AttrName::printer_info,
                                                 AttrType::text};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<std::string> printer_more_info{AttrName::printer_more_info,
                                               AttrType::uri};
    SingleValue<E_printer_state> printer_state{AttrName::printer_state,
                                               AttrType::enum_};
    SingleValue<StringWithLanguage> printer_state_message{
        AttrName::printer_state_message, AttrType::text};
    SetOfValues<StringWithLanguage> requesting_user_name_allowed{
        AttrName::requesting_user_name_allowed, AttrType::name};
    SetOfValues<StringWithLanguage> requesting_user_name_denied{
        AttrName::requesting_user_name_denied, AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Request_CUPS_Add_Modify_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Add_Modify_Printer : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Add_Modify_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Authenticate_Job : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_job_attributes : public Collection {
    SetOfValues<StringWithLanguage> auth_info{AttrName::auth_info,
                                              AttrType::text};
    SingleValue<E_job_hold_until> job_hold_until{AttrName::job_hold_until,
                                                 AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_CUPS_Authenticate_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Authenticate_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_unsupported_attributes : public Collection {
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_CUPS_Authenticate_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Create_Local_Printer : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    SingleValue<StringWithLanguage> printer_name{AttrName::printer_name,
                                                 AttrType::name};
    SingleValue<std::string> device_uri{AttrName::device_uri, AttrType::uri};
    SingleValue<StringWithLanguage> printer_device_id{
        AttrName::printer_device_id, AttrType::text};
    SingleValue<std::string> printer_geo_location{
        AttrName::printer_geo_location, AttrType::uri};
    SingleValue<StringWithLanguage> printer_info{AttrName::printer_info,
                                                 AttrType::text};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Request_CUPS_Create_Local_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Create_Local_Printer : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    SingleValue<int> printer_id{AttrName::printer_id, AttrType::integer};
    SingleValue<bool> printer_is_accepting_jobs{
        AttrName::printer_is_accepting_jobs, AttrType::boolean};
    SingleValue<E_printer_state> printer_state{AttrName::printer_state,
                                               AttrType::enum_};
    SetOfValues<E_printer_state_reasons> printer_state_reasons{
        AttrName::printer_state_reasons, AttrType::keyword};
    SetOfValues<std::string> printer_uri_supported{
        AttrName::printer_uri_supported, AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Response_CUPS_Create_Local_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Delete_Class : public Request {
  typedef Request_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Delete_Class();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Delete_Class : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Delete_Class();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Delete_Printer : public Request {
  typedef Request_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Delete_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Delete_Printer : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Delete_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Get_Classes : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<StringWithLanguage> first_printer_name{
        AttrName::first_printer_name, AttrType::name};
    SingleValue<int> limit{AttrName::limit, AttrType::integer};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<int> printer_type{AttrName::printer_type, AttrType::integer};
    SingleValue<int> printer_type_mask{AttrName::printer_type_mask,
                                       AttrType::integer};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    SingleValue<StringWithLanguage> requested_user_name{
        AttrName::requested_user_name, AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Get_Classes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Get_Classes : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    SetOfValues<StringWithLanguage> member_names{AttrName::member_names,
                                                 AttrType::name};
    SetOfValues<std::string> member_uris{AttrName::member_uris, AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Response_CUPS_Get_Classes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Get_Default : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Get_Default();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Get_Default : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_printer_attributes : public Collection {
    OpenSetOfValues<E_baling_type_supported> baling_type_supported{
        AttrName::baling_type_supported, AttrType::keyword};
    SetOfValues<E_baling_when_supported> baling_when_supported{
        AttrName::baling_when_supported, AttrType::keyword};
    SetOfValues<E_binding_reference_edge_supported>
        binding_reference_edge_supported{
            AttrName::binding_reference_edge_supported, AttrType::keyword};
    SetOfValues<E_binding_type_supported> binding_type_supported{
        AttrName::binding_type_supported, AttrType::keyword};
    SingleValue<std::string> charset_configured{AttrName::charset_configured,
                                                AttrType::charset};
    SetOfValues<std::string> charset_supported{AttrName::charset_supported,
                                               AttrType::charset};
    SetOfValues<E_coating_sides_supported> coating_sides_supported{
        AttrName::coating_sides_supported, AttrType::keyword};
    OpenSetOfValues<E_coating_type_supported> coating_type_supported{
        AttrName::coating_type_supported, AttrType::keyword};
    SingleValue<bool> color_supported{AttrName::color_supported,
                                      AttrType::boolean};
    SetOfValues<E_compression_supported> compression_supported{
        AttrName::compression_supported, AttrType::keyword};
    SingleValue<int> copies_default{AttrName::copies_default,
                                    AttrType::integer};
    SingleValue<RangeOfInteger> copies_supported{AttrName::copies_supported,
                                                 AttrType::rangeOfInteger};
    SingleCollection<C_cover_back_default> cover_back_default{
        AttrName::cover_back_default};
    SetOfValues<E_cover_back_supported> cover_back_supported{
        AttrName::cover_back_supported, AttrType::keyword};
    SingleCollection<C_cover_front_default> cover_front_default{
        AttrName::cover_front_default};
    SetOfValues<E_cover_front_supported> cover_front_supported{
        AttrName::cover_front_supported, AttrType::keyword};
    OpenSetOfValues<E_covering_name_supported> covering_name_supported{
        AttrName::covering_name_supported, AttrType::keyword};
    SingleValue<std::string> document_charset_default{
        AttrName::document_charset_default, AttrType::charset};
    SetOfValues<std::string> document_charset_supported{
        AttrName::document_charset_supported, AttrType::charset};
    SingleValue<E_document_digital_signature_default>
        document_digital_signature_default{
            AttrName::document_digital_signature_default, AttrType::keyword};
    SetOfValues<E_document_digital_signature_supported>
        document_digital_signature_supported{
            AttrName::document_digital_signature_supported, AttrType::keyword};
    SingleValue<std::string> document_format_default{
        AttrName::document_format_default, AttrType::mimeMediaType};
    SingleCollection<C_document_format_details_default>
        document_format_details_default{
            AttrName::document_format_details_default};
    SetOfValues<E_document_format_details_supported>
        document_format_details_supported{
            AttrName::document_format_details_supported, AttrType::keyword};
    SetOfValues<std::string> document_format_supported{
        AttrName::document_format_supported, AttrType::mimeMediaType};
    SingleValue<StringWithLanguage> document_format_version_default{
        AttrName::document_format_version_default, AttrType::text};
    SetOfValues<StringWithLanguage> document_format_version_supported{
        AttrName::document_format_version_supported, AttrType::text};
    SingleValue<std::string> document_natural_language_default{
        AttrName::document_natural_language_default, AttrType::naturalLanguage};
    SetOfValues<std::string> document_natural_language_supported{
        AttrName::document_natural_language_supported,
        AttrType::naturalLanguage};
    SingleValue<int> document_password_supported{
        AttrName::document_password_supported, AttrType::integer};
    SetOfValues<E_feed_orientation_supported> feed_orientation_supported{
        AttrName::feed_orientation_supported, AttrType::keyword};
    OpenSetOfValues<E_finishing_template_supported>
        finishing_template_supported{AttrName::finishing_template_supported,
                                     AttrType::keyword};
    SetOfCollections<C_finishings_col_database> finishings_col_database{
        AttrName::finishings_col_database};
    SetOfCollections<C_finishings_col_default> finishings_col_default{
        AttrName::finishings_col_default};
    SetOfCollections<C_finishings_col_ready> finishings_col_ready{
        AttrName::finishings_col_ready};
    SetOfValues<E_finishings_default> finishings_default{
        AttrName::finishings_default, AttrType::enum_};
    SetOfValues<E_finishings_ready> finishings_ready{AttrName::finishings_ready,
                                                     AttrType::enum_};
    SetOfValues<E_finishings_supported> finishings_supported{
        AttrName::finishings_supported, AttrType::enum_};
    SetOfValues<E_folding_direction_supported> folding_direction_supported{
        AttrName::folding_direction_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> folding_offset_supported{
        AttrName::folding_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_folding_reference_edge_supported>
        folding_reference_edge_supported{
            AttrName::folding_reference_edge_supported, AttrType::keyword};
    SingleValue<StringWithLanguage> font_name_requested_default{
        AttrName::font_name_requested_default, AttrType::name};
    SetOfValues<StringWithLanguage> font_name_requested_supported{
        AttrName::font_name_requested_supported, AttrType::name};
    SingleValue<int> font_size_requested_default{
        AttrName::font_size_requested_default, AttrType::integer};
    SetOfValues<RangeOfInteger> font_size_requested_supported{
        AttrName::font_size_requested_supported, AttrType::rangeOfInteger};
    SetOfValues<std::string> generated_natural_language_supported{
        AttrName::generated_natural_language_supported,
        AttrType::naturalLanguage};
    SetOfValues<E_identify_actions_default> identify_actions_default{
        AttrName::identify_actions_default, AttrType::keyword};
    SetOfValues<E_identify_actions_supported> identify_actions_supported{
        AttrName::identify_actions_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> insert_after_page_number_supported{
        AttrName::insert_after_page_number_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> insert_count_supported{
        AttrName::insert_count_supported, AttrType::rangeOfInteger};
    SetOfCollections<C_insert_sheet_default> insert_sheet_default{
        AttrName::insert_sheet_default};
    SetOfValues<E_ipp_features_supported> ipp_features_supported{
        AttrName::ipp_features_supported, AttrType::keyword};
    SetOfValues<E_ipp_versions_supported> ipp_versions_supported{
        AttrName::ipp_versions_supported, AttrType::keyword};
    SingleValue<int> ippget_event_life{AttrName::ippget_event_life,
                                       AttrType::integer};
    SingleValue<StringWithLanguage> job_account_id_default{
        AttrName::job_account_id_default, AttrType::name};
    SingleValue<bool> job_account_id_supported{
        AttrName::job_account_id_supported, AttrType::boolean};
    SingleValue<E_job_account_type_default> job_account_type_default{
        AttrName::job_account_type_default, AttrType::keyword};
    OpenSetOfValues<E_job_account_type_supported> job_account_type_supported{
        AttrName::job_account_type_supported, AttrType::keyword};
    SingleCollection<C_job_accounting_sheets_default>
        job_accounting_sheets_default{AttrName::job_accounting_sheets_default};
    SingleValue<StringWithLanguage> job_accounting_user_id_default{
        AttrName::job_accounting_user_id_default, AttrType::name};
    SingleValue<bool> job_accounting_user_id_supported{
        AttrName::job_accounting_user_id_supported, AttrType::boolean};
    SingleValue<bool> job_authorization_uri_supported{
        AttrName::job_authorization_uri_supported, AttrType::boolean};
    SetOfCollections<C_job_constraints_supported> job_constraints_supported{
        AttrName::job_constraints_supported};
    SingleValue<int> job_copies_default{AttrName::job_copies_default,
                                        AttrType::integer};
    SingleValue<RangeOfInteger> job_copies_supported{
        AttrName::job_copies_supported, AttrType::rangeOfInteger};
    SingleCollection<C_job_cover_back_default> job_cover_back_default{
        AttrName::job_cover_back_default};
    SetOfValues<E_job_cover_back_supported> job_cover_back_supported{
        AttrName::job_cover_back_supported, AttrType::keyword};
    SingleCollection<C_job_cover_front_default> job_cover_front_default{
        AttrName::job_cover_front_default};
    SetOfValues<E_job_cover_front_supported> job_cover_front_supported{
        AttrName::job_cover_front_supported, AttrType::keyword};
    SingleValue<E_job_delay_output_until_default>
        job_delay_output_until_default{AttrName::job_delay_output_until_default,
                                       AttrType::keyword};
    OpenSetOfValues<E_job_delay_output_until_supported>
        job_delay_output_until_supported{
            AttrName::job_delay_output_until_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> job_delay_output_until_time_supported{
        AttrName::job_delay_output_until_time_supported,
        AttrType::rangeOfInteger};
    SingleValue<E_job_error_action_default> job_error_action_default{
        AttrName::job_error_action_default, AttrType::keyword};
    SetOfValues<E_job_error_action_supported> job_error_action_supported{
        AttrName::job_error_action_supported, AttrType::keyword};
    SingleCollection<C_job_error_sheet_default> job_error_sheet_default{
        AttrName::job_error_sheet_default};
    SetOfCollections<C_job_finishings_col_default> job_finishings_col_default{
        AttrName::job_finishings_col_default};
    SetOfCollections<C_job_finishings_col_ready> job_finishings_col_ready{
        AttrName::job_finishings_col_ready};
    SetOfValues<E_job_finishings_default> job_finishings_default{
        AttrName::job_finishings_default, AttrType::enum_};
    SetOfValues<E_job_finishings_ready> job_finishings_ready{
        AttrName::job_finishings_ready, AttrType::enum_};
    SetOfValues<E_job_finishings_supported> job_finishings_supported{
        AttrName::job_finishings_supported, AttrType::enum_};
    SingleValue<E_job_hold_until_default> job_hold_until_default{
        AttrName::job_hold_until_default, AttrType::keyword};
    OpenSetOfValues<E_job_hold_until_supported> job_hold_until_supported{
        AttrName::job_hold_until_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> job_hold_until_time_supported{
        AttrName::job_hold_until_time_supported, AttrType::rangeOfInteger};
    SingleValue<bool> job_ids_supported{AttrName::job_ids_supported,
                                        AttrType::boolean};
    SingleValue<RangeOfInteger> job_impressions_supported{
        AttrName::job_impressions_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> job_k_octets_supported{
        AttrName::job_k_octets_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> job_media_sheets_supported{
        AttrName::job_media_sheets_supported, AttrType::rangeOfInteger};
    SingleValue<StringWithLanguage> job_message_to_operator_default{
        AttrName::job_message_to_operator_default, AttrType::text};
    SingleValue<bool> job_message_to_operator_supported{
        AttrName::job_message_to_operator_supported, AttrType::boolean};
    SingleValue<bool> job_pages_per_set_supported{
        AttrName::job_pages_per_set_supported, AttrType::boolean};
    OpenSetOfValues<E_job_password_encryption_supported>
        job_password_encryption_supported{
            AttrName::job_password_encryption_supported, AttrType::keyword};
    SingleValue<int> job_password_supported{AttrName::job_password_supported,
                                            AttrType::integer};
    SingleValue<std::string> job_phone_number_default{
        AttrName::job_phone_number_default, AttrType::uri};
    SingleValue<bool> job_phone_number_supported{
        AttrName::job_phone_number_supported, AttrType::boolean};
    SingleValue<int> job_priority_default{AttrName::job_priority_default,
                                          AttrType::integer};
    SingleValue<int> job_priority_supported{AttrName::job_priority_supported,
                                            AttrType::integer};
    SingleValue<StringWithLanguage> job_recipient_name_default{
        AttrName::job_recipient_name_default, AttrType::name};
    SingleValue<bool> job_recipient_name_supported{
        AttrName::job_recipient_name_supported, AttrType::boolean};
    SetOfCollections<C_job_resolvers_supported> job_resolvers_supported{
        AttrName::job_resolvers_supported};
    SingleValue<StringWithLanguage> job_sheet_message_default{
        AttrName::job_sheet_message_default, AttrType::text};
    SingleValue<bool> job_sheet_message_supported{
        AttrName::job_sheet_message_supported, AttrType::boolean};
    SingleCollection<C_job_sheets_col_default> job_sheets_col_default{
        AttrName::job_sheets_col_default};
    SingleValue<E_job_sheets_default> job_sheets_default{
        AttrName::job_sheets_default, AttrType::keyword};
    OpenSetOfValues<E_job_sheets_supported> job_sheets_supported{
        AttrName::job_sheets_supported, AttrType::keyword};
    SingleValue<E_job_spooling_supported> job_spooling_supported{
        AttrName::job_spooling_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> jpeg_k_octets_supported{
        AttrName::jpeg_k_octets_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> jpeg_x_dimension_supported{
        AttrName::jpeg_x_dimension_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> jpeg_y_dimension_supported{
        AttrName::jpeg_y_dimension_supported, AttrType::rangeOfInteger};
    SetOfValues<E_laminating_sides_supported> laminating_sides_supported{
        AttrName::laminating_sides_supported, AttrType::keyword};
    OpenSetOfValues<E_laminating_type_supported> laminating_type_supported{
        AttrName::laminating_type_supported, AttrType::keyword};
    SingleValue<int> max_save_info_supported{AttrName::max_save_info_supported,
                                             AttrType::integer};
    SingleValue<int> max_stitching_locations_supported{
        AttrName::max_stitching_locations_supported, AttrType::integer};
    OpenSetOfValues<E_media_back_coating_supported>
        media_back_coating_supported{AttrName::media_back_coating_supported,
                                     AttrType::keyword};
    SetOfValues<int> media_bottom_margin_supported{
        AttrName::media_bottom_margin_supported, AttrType::integer};
    SetOfCollections<C_media_col_database> media_col_database{
        AttrName::media_col_database};
    SingleCollection<C_media_col_default> media_col_default{
        AttrName::media_col_default};
    SetOfCollections<C_media_col_ready> media_col_ready{
        AttrName::media_col_ready};
    OpenSetOfValues<E_media_color_supported> media_color_supported{
        AttrName::media_color_supported, AttrType::keyword};
    SingleValue<E_media_default> media_default{AttrName::media_default,
                                               AttrType::keyword};
    OpenSetOfValues<E_media_front_coating_supported>
        media_front_coating_supported{AttrName::media_front_coating_supported,
                                      AttrType::keyword};
    OpenSetOfValues<E_media_grain_supported> media_grain_supported{
        AttrName::media_grain_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> media_hole_count_supported{
        AttrName::media_hole_count_supported, AttrType::rangeOfInteger};
    SingleValue<bool> media_info_supported{AttrName::media_info_supported,
                                           AttrType::boolean};
    SetOfValues<int> media_left_margin_supported{
        AttrName::media_left_margin_supported, AttrType::integer};
    SetOfValues<RangeOfInteger> media_order_count_supported{
        AttrName::media_order_count_supported, AttrType::rangeOfInteger};
    OpenSetOfValues<E_media_pre_printed_supported> media_pre_printed_supported{
        AttrName::media_pre_printed_supported, AttrType::keyword};
    OpenSetOfValues<E_media_ready> media_ready{AttrName::media_ready,
                                               AttrType::keyword};
    OpenSetOfValues<E_media_recycled_supported> media_recycled_supported{
        AttrName::media_recycled_supported, AttrType::keyword};
    SetOfValues<int> media_right_margin_supported{
        AttrName::media_right_margin_supported, AttrType::integer};
    SetOfCollections<C_media_size_supported> media_size_supported{
        AttrName::media_size_supported};
    OpenSetOfValues<E_media_source_supported> media_source_supported{
        AttrName::media_source_supported, AttrType::keyword};
    OpenSetOfValues<E_media_supported> media_supported{
        AttrName::media_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> media_thickness_supported{
        AttrName::media_thickness_supported, AttrType::rangeOfInteger};
    OpenSetOfValues<E_media_tooth_supported> media_tooth_supported{
        AttrName::media_tooth_supported, AttrType::keyword};
    SetOfValues<int> media_top_margin_supported{
        AttrName::media_top_margin_supported, AttrType::integer};
    OpenSetOfValues<E_media_type_supported> media_type_supported{
        AttrName::media_type_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> media_weight_metric_supported{
        AttrName::media_weight_metric_supported, AttrType::rangeOfInteger};
    SingleValue<E_multiple_document_handling_default>
        multiple_document_handling_default{
            AttrName::multiple_document_handling_default, AttrType::keyword};
    SetOfValues<E_multiple_document_handling_supported>
        multiple_document_handling_supported{
            AttrName::multiple_document_handling_supported, AttrType::keyword};
    SingleValue<bool> multiple_document_jobs_supported{
        AttrName::multiple_document_jobs_supported, AttrType::boolean};
    SingleValue<int> multiple_operation_time_out{
        AttrName::multiple_operation_time_out, AttrType::integer};
    SingleValue<E_multiple_operation_time_out_action>
        multiple_operation_time_out_action{
            AttrName::multiple_operation_time_out_action, AttrType::keyword};
    SingleValue<std::string> natural_language_configured{
        AttrName::natural_language_configured, AttrType::naturalLanguage};
    SetOfValues<E_notify_events_default> notify_events_default{
        AttrName::notify_events_default, AttrType::keyword};
    SetOfValues<E_notify_events_supported> notify_events_supported{
        AttrName::notify_events_supported, AttrType::keyword};
    SingleValue<int> notify_lease_duration_default{
        AttrName::notify_lease_duration_default, AttrType::integer};
    SetOfValues<RangeOfInteger> notify_lease_duration_supported{
        AttrName::notify_lease_duration_supported, AttrType::rangeOfInteger};
    SetOfValues<E_notify_pull_method_supported> notify_pull_method_supported{
        AttrName::notify_pull_method_supported, AttrType::keyword};
    SetOfValues<std::string> notify_schemes_supported{
        AttrName::notify_schemes_supported, AttrType::uriScheme};
    SingleValue<int> number_up_default{AttrName::number_up_default,
                                       AttrType::integer};
    SingleValue<RangeOfInteger> number_up_supported{
        AttrName::number_up_supported, AttrType::rangeOfInteger};
    SingleValue<std::string> oauth_authorization_server_uri{
        AttrName::oauth_authorization_server_uri, AttrType::uri};
    SetOfValues<E_operations_supported> operations_supported{
        AttrName::operations_supported, AttrType::enum_};
    SingleValue<E_orientation_requested_default> orientation_requested_default{
        AttrName::orientation_requested_default, AttrType::enum_};
    SetOfValues<E_orientation_requested_supported>
        orientation_requested_supported{
            AttrName::orientation_requested_supported, AttrType::enum_};
    SingleValue<E_output_bin_default> output_bin_default{
        AttrName::output_bin_default, AttrType::keyword};
    OpenSetOfValues<E_output_bin_supported> output_bin_supported{
        AttrName::output_bin_supported, AttrType::keyword};
    SetOfValues<StringWithLanguage> output_device_supported{
        AttrName::output_device_supported, AttrType::name};
    SetOfValues<std::string> output_device_uuid_supported{
        AttrName::output_device_uuid_supported, AttrType::uri};
    SingleValue<E_page_delivery_default> page_delivery_default{
        AttrName::page_delivery_default, AttrType::keyword};
    SetOfValues<E_page_delivery_supported> page_delivery_supported{
        AttrName::page_delivery_supported, AttrType::keyword};
    SingleValue<E_page_order_received_default> page_order_received_default{
        AttrName::page_order_received_default, AttrType::keyword};
    SetOfValues<E_page_order_received_supported> page_order_received_supported{
        AttrName::page_order_received_supported, AttrType::keyword};
    SingleValue<bool> page_ranges_supported{AttrName::page_ranges_supported,
                                            AttrType::boolean};
    SingleValue<bool> pages_per_subset_supported{
        AttrName::pages_per_subset_supported, AttrType::boolean};
    SetOfValues<std::string> parent_printers_supported{
        AttrName::parent_printers_supported, AttrType::uri};
    SingleValue<RangeOfInteger> pdf_k_octets_supported{
        AttrName::pdf_k_octets_supported, AttrType::rangeOfInteger};
    SetOfValues<E_pdf_versions_supported> pdf_versions_supported{
        AttrName::pdf_versions_supported, AttrType::keyword};
    SingleCollection<C_pdl_init_file_default> pdl_init_file_default{
        AttrName::pdl_init_file_default};
    SetOfValues<StringWithLanguage> pdl_init_file_entry_supported{
        AttrName::pdl_init_file_entry_supported, AttrType::name};
    SetOfValues<std::string> pdl_init_file_location_supported{
        AttrName::pdl_init_file_location_supported, AttrType::uri};
    SingleValue<bool> pdl_init_file_name_subdirectory_supported{
        AttrName::pdl_init_file_name_subdirectory_supported, AttrType::boolean};
    SetOfValues<StringWithLanguage> pdl_init_file_name_supported{
        AttrName::pdl_init_file_name_supported, AttrType::name};
    SetOfValues<E_pdl_init_file_supported> pdl_init_file_supported{
        AttrName::pdl_init_file_supported, AttrType::keyword};
    SingleValue<E_pdl_override_supported> pdl_override_supported{
        AttrName::pdl_override_supported, AttrType::keyword};
    SingleValue<bool> preferred_attributes_supported{
        AttrName::preferred_attributes_supported, AttrType::boolean};
    SingleValue<E_presentation_direction_number_up_default>
        presentation_direction_number_up_default{
            AttrName::presentation_direction_number_up_default,
            AttrType::keyword};
    SetOfValues<E_presentation_direction_number_up_supported>
        presentation_direction_number_up_supported{
            AttrName::presentation_direction_number_up_supported,
            AttrType::keyword};
    SingleValue<E_print_color_mode_default> print_color_mode_default{
        AttrName::print_color_mode_default, AttrType::keyword};
    SetOfValues<E_print_color_mode_supported> print_color_mode_supported{
        AttrName::print_color_mode_supported, AttrType::keyword};
    SingleValue<E_print_content_optimize_default>
        print_content_optimize_default{AttrName::print_content_optimize_default,
                                       AttrType::keyword};
    SetOfValues<E_print_content_optimize_supported>
        print_content_optimize_supported{
            AttrName::print_content_optimize_supported, AttrType::keyword};
    SingleValue<E_print_quality_default> print_quality_default{
        AttrName::print_quality_default, AttrType::enum_};
    SetOfValues<E_print_quality_supported> print_quality_supported{
        AttrName::print_quality_supported, AttrType::enum_};
    SingleValue<E_print_rendering_intent_default>
        print_rendering_intent_default{AttrName::print_rendering_intent_default,
                                       AttrType::keyword};
    SetOfValues<E_print_rendering_intent_supported>
        print_rendering_intent_supported{
            AttrName::print_rendering_intent_supported, AttrType::keyword};
    SingleValue<StringWithLanguage> printer_charge_info{
        AttrName::printer_charge_info, AttrType::text};
    SingleValue<std::string> printer_charge_info_uri{
        AttrName::printer_charge_info_uri, AttrType::uri};
    SingleCollection<C_printer_contact_col> printer_contact_col{
        AttrName::printer_contact_col};
    SingleValue<DateTime> printer_current_time{AttrName::printer_current_time,
                                               AttrType::dateTime};
    SingleValue<StringWithLanguage> printer_device_id{
        AttrName::printer_device_id, AttrType::text};
    SingleValue<StringWithLanguage> printer_dns_sd_name{
        AttrName::printer_dns_sd_name, AttrType::name};
    SingleValue<std::string> printer_driver_installer{
        AttrName::printer_driver_installer, AttrType::uri};
    SingleValue<std::string> printer_geo_location{
        AttrName::printer_geo_location, AttrType::uri};
    SetOfCollections<C_printer_icc_profiles> printer_icc_profiles{
        AttrName::printer_icc_profiles};
    SetOfValues<std::string> printer_icons{AttrName::printer_icons,
                                           AttrType::uri};
    SingleValue<StringWithLanguage> printer_info{AttrName::printer_info,
                                                 AttrType::text};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<StringWithLanguage> printer_make_and_model{
        AttrName::printer_make_and_model, AttrType::text};
    SingleValue<std::string> printer_more_info_manufacturer{
        AttrName::printer_more_info_manufacturer, AttrType::uri};
    SingleValue<StringWithLanguage> printer_name{AttrName::printer_name,
                                                 AttrType::name};
    SetOfValues<StringWithLanguage> printer_organization{
        AttrName::printer_organization, AttrType::text};
    SetOfValues<StringWithLanguage> printer_organizational_unit{
        AttrName::printer_organizational_unit, AttrType::text};
    SingleValue<Resolution> printer_resolution_default{
        AttrName::printer_resolution_default, AttrType::resolution};
    SingleValue<Resolution> printer_resolution_supported{
        AttrName::printer_resolution_supported, AttrType::resolution};
    SingleValue<std::string> printer_static_resource_directory_uri{
        AttrName::printer_static_resource_directory_uri, AttrType::uri};
    SingleValue<int> printer_static_resource_k_octets_supported{
        AttrName::printer_static_resource_k_octets_supported,
        AttrType::integer};
    SetOfValues<std::string> printer_strings_languages_supported{
        AttrName::printer_strings_languages_supported,
        AttrType::naturalLanguage};
    SingleValue<std::string> printer_strings_uri{AttrName::printer_strings_uri,
                                                 AttrType::uri};
    SetOfCollections<C_printer_xri_supported> printer_xri_supported{
        AttrName::printer_xri_supported};
    SingleCollection<C_proof_print_default> proof_print_default{
        AttrName::proof_print_default};
    SetOfValues<E_proof_print_supported> proof_print_supported{
        AttrName::proof_print_supported, AttrType::keyword};
    SingleValue<int> punching_hole_diameter_configured{
        AttrName::punching_hole_diameter_configured, AttrType::integer};
    SetOfValues<RangeOfInteger> punching_locations_supported{
        AttrName::punching_locations_supported, AttrType::rangeOfInteger};
    SetOfValues<RangeOfInteger> punching_offset_supported{
        AttrName::punching_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_punching_reference_edge_supported>
        punching_reference_edge_supported{
            AttrName::punching_reference_edge_supported, AttrType::keyword};
    SetOfValues<Resolution> pwg_raster_document_resolution_supported{
        AttrName::pwg_raster_document_resolution_supported,
        AttrType::resolution};
    SingleValue<E_pwg_raster_document_sheet_back>
        pwg_raster_document_sheet_back{AttrName::pwg_raster_document_sheet_back,
                                       AttrType::keyword};
    SetOfValues<E_pwg_raster_document_type_supported>
        pwg_raster_document_type_supported{
            AttrName::pwg_raster_document_type_supported, AttrType::keyword};
    SetOfValues<std::string> reference_uri_schemes_supported{
        AttrName::reference_uri_schemes_supported, AttrType::uriScheme};
    SingleValue<bool> requesting_user_uri_supported{
        AttrName::requesting_user_uri_supported, AttrType::boolean};
    SetOfValues<E_save_disposition_supported> save_disposition_supported{
        AttrName::save_disposition_supported, AttrType::keyword};
    SingleValue<std::string> save_document_format_default{
        AttrName::save_document_format_default, AttrType::mimeMediaType};
    SetOfValues<std::string> save_document_format_supported{
        AttrName::save_document_format_supported, AttrType::mimeMediaType};
    SingleValue<std::string> save_location_default{
        AttrName::save_location_default, AttrType::uri};
    SetOfValues<std::string> save_location_supported{
        AttrName::save_location_supported, AttrType::uri};
    SingleValue<bool> save_name_subdirectory_supported{
        AttrName::save_name_subdirectory_supported, AttrType::boolean};
    SingleValue<bool> save_name_supported{AttrName::save_name_supported,
                                          AttrType::boolean};
    SingleCollection<C_separator_sheets_default> separator_sheets_default{
        AttrName::separator_sheets_default};
    SingleValue<E_sheet_collate_default> sheet_collate_default{
        AttrName::sheet_collate_default, AttrType::keyword};
    SetOfValues<E_sheet_collate_supported> sheet_collate_supported{
        AttrName::sheet_collate_supported, AttrType::keyword};
    SingleValue<E_sides_default> sides_default{AttrName::sides_default,
                                               AttrType::keyword};
    SetOfValues<E_sides_supported> sides_supported{AttrName::sides_supported,
                                                   AttrType::keyword};
    SetOfValues<RangeOfInteger> stitching_angle_supported{
        AttrName::stitching_angle_supported, AttrType::rangeOfInteger};
    SetOfValues<RangeOfInteger> stitching_locations_supported{
        AttrName::stitching_locations_supported, AttrType::rangeOfInteger};
    SetOfValues<E_stitching_method_supported> stitching_method_supported{
        AttrName::stitching_method_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> stitching_offset_supported{
        AttrName::stitching_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_stitching_reference_edge_supported>
        stitching_reference_edge_supported{
            AttrName::stitching_reference_edge_supported, AttrType::keyword};
    SetOfValues<std::string> subordinate_printers_supported{
        AttrName::subordinate_printers_supported, AttrType::uri};
    SetOfValues<RangeOfInteger> trimming_offset_supported{
        AttrName::trimming_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_trimming_reference_edge_supported>
        trimming_reference_edge_supported{
            AttrName::trimming_reference_edge_supported, AttrType::keyword};
    SetOfValues<E_trimming_type_supported> trimming_type_supported{
        AttrName::trimming_type_supported, AttrType::keyword};
    SetOfValues<E_trimming_when_supported> trimming_when_supported{
        AttrName::trimming_when_supported, AttrType::keyword};
    SetOfValues<E_uri_authentication_supported> uri_authentication_supported{
        AttrName::uri_authentication_supported, AttrType::keyword};
    SetOfValues<E_uri_security_supported> uri_security_supported{
        AttrName::uri_security_supported, AttrType::keyword};
    SetOfValues<E_which_jobs_supported> which_jobs_supported{
        AttrName::which_jobs_supported, AttrType::keyword};
    SingleValue<E_x_image_position_default> x_image_position_default{
        AttrName::x_image_position_default, AttrType::keyword};
    SetOfValues<E_x_image_position_supported> x_image_position_supported{
        AttrName::x_image_position_supported, AttrType::keyword};
    SingleValue<int> x_image_shift_default{AttrName::x_image_shift_default,
                                           AttrType::integer};
    SingleValue<RangeOfInteger> x_image_shift_supported{
        AttrName::x_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> x_side1_image_shift_default{
        AttrName::x_side1_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> x_side1_image_shift_supported{
        AttrName::x_side1_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> x_side2_image_shift_default{
        AttrName::x_side2_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> x_side2_image_shift_supported{
        AttrName::x_side2_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<E_y_image_position_default> y_image_position_default{
        AttrName::y_image_position_default, AttrType::keyword};
    SetOfValues<E_y_image_position_supported> y_image_position_supported{
        AttrName::y_image_position_supported, AttrType::keyword};
    SingleValue<int> y_image_shift_default{AttrName::y_image_shift_default,
                                           AttrType::integer};
    SingleValue<RangeOfInteger> y_image_shift_supported{
        AttrName::y_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> y_side1_image_shift_default{
        AttrName::y_side1_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> y_side1_image_shift_supported{
        AttrName::y_side1_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> y_side2_image_shift_default{
        AttrName::y_side2_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> y_side2_image_shift_supported{
        AttrName::y_side2_image_shift_supported, AttrType::rangeOfInteger};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Response_CUPS_Get_Default();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Get_Document : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<int> document_number{AttrName::document_number,
                                     AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Get_Document();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Get_Document : public Response {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<StringWithLanguage> status_message{AttrName::status_message,
                                                   AttrType::text};
    SingleValue<StringWithLanguage> detailed_status_message{
        AttrName::detailed_status_message, AttrType::text};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    SingleValue<int> document_number{AttrName::document_number,
                                     AttrType::integer};
    SingleValue<StringWithLanguage> document_name{AttrName::document_name,
                                                  AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Get_Document();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Get_Printers : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<StringWithLanguage> first_printer_name{
        AttrName::first_printer_name, AttrType::name};
    SingleValue<int> limit{AttrName::limit, AttrType::integer};
    SingleValue<int> printer_id{AttrName::printer_id, AttrType::integer};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<int> printer_type{AttrName::printer_type, AttrType::integer};
    SingleValue<int> printer_type_mask{AttrName::printer_type_mask,
                                       AttrType::integer};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    SingleValue<StringWithLanguage> requested_user_name{
        AttrName::requested_user_name, AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Get_Printers();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Get_Printers : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Get_Default::G_printer_attributes G_printer_attributes;
  SetOfGroups<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Response_CUPS_Get_Printers();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Move_Job : public Request {
  typedef Request_CUPS_Authenticate_Job::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_job_attributes : public Collection {
    SingleValue<std::string> job_printer_uri{AttrName::job_printer_uri,
                                             AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_CUPS_Move_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Move_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Move_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_CUPS_Set_Default : public Request {
  typedef Request_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_CUPS_Set_Default();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_CUPS_Set_Default : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Response_CUPS_Set_Default();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Cancel_Job : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> message{AttrName::message, AttrType::text};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Cancel_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Cancel_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Cancel_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Create_Job : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> job_name{AttrName::job_name,
                                             AttrType::name};
    SingleValue<bool> ipp_attribute_fidelity{AttrName::ipp_attribute_fidelity,
                                             AttrType::boolean};
    SingleValue<int> job_k_octets{AttrName::job_k_octets, AttrType::integer};
    SingleValue<int> job_impressions{AttrName::job_impressions,
                                     AttrType::integer};
    SingleValue<int> job_media_sheets{AttrName::job_media_sheets,
                                      AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  struct G_job_attributes : public Collection {
    SingleValue<int> copies{AttrName::copies, AttrType::integer};
    SingleCollection<C_cover_back> cover_back{AttrName::cover_back};
    SingleCollection<C_cover_front> cover_front{AttrName::cover_front};
    SingleValue<E_feed_orientation> feed_orientation{AttrName::feed_orientation,
                                                     AttrType::keyword};
    SetOfValues<E_finishings> finishings{AttrName::finishings, AttrType::enum_};
    SetOfCollections<C_finishings_col> finishings_col{AttrName::finishings_col};
    SingleValue<StringWithLanguage> font_name_requested{
        AttrName::font_name_requested, AttrType::name};
    SingleValue<int> font_size_requested{AttrName::font_size_requested,
                                         AttrType::integer};
    SetOfValues<int> force_front_side{AttrName::force_front_side,
                                      AttrType::integer};
    SingleValue<E_imposition_template> imposition_template{
        AttrName::imposition_template, AttrType::keyword};
    SetOfCollections<C_insert_sheet> insert_sheet{AttrName::insert_sheet};
    SingleValue<StringWithLanguage> job_account_id{AttrName::job_account_id,
                                                   AttrType::name};
    SingleValue<E_job_account_type> job_account_type{AttrName::job_account_type,
                                                     AttrType::keyword};
    SingleCollection<C_job_accounting_sheets> job_accounting_sheets{
        AttrName::job_accounting_sheets};
    SingleValue<StringWithLanguage> job_accounting_user_id{
        AttrName::job_accounting_user_id, AttrType::name};
    SingleValue<int> job_copies{AttrName::job_copies, AttrType::integer};
    SingleCollection<C_job_cover_back> job_cover_back{AttrName::job_cover_back};
    SingleCollection<C_job_cover_front> job_cover_front{
        AttrName::job_cover_front};
    SingleValue<E_job_delay_output_until> job_delay_output_until{
        AttrName::job_delay_output_until, AttrType::keyword};
    SingleValue<DateTime> job_delay_output_until_time{
        AttrName::job_delay_output_until_time, AttrType::dateTime};
    SingleValue<E_job_error_action> job_error_action{AttrName::job_error_action,
                                                     AttrType::keyword};
    SingleCollection<C_job_error_sheet> job_error_sheet{
        AttrName::job_error_sheet};
    SetOfValues<E_job_finishings> job_finishings{AttrName::job_finishings,
                                                 AttrType::enum_};
    SetOfCollections<C_job_finishings_col> job_finishings_col{
        AttrName::job_finishings_col};
    SingleValue<E_job_hold_until> job_hold_until{AttrName::job_hold_until,
                                                 AttrType::keyword};
    SingleValue<DateTime> job_hold_until_time{AttrName::job_hold_until_time,
                                              AttrType::dateTime};
    SingleValue<StringWithLanguage> job_message_to_operator{
        AttrName::job_message_to_operator, AttrType::text};
    SingleValue<int> job_pages_per_set{AttrName::job_pages_per_set,
                                       AttrType::integer};
    SingleValue<std::string> job_phone_number{AttrName::job_phone_number,
                                              AttrType::uri};
    SingleValue<int> job_priority{AttrName::job_priority, AttrType::integer};
    SingleValue<StringWithLanguage> job_recipient_name{
        AttrName::job_recipient_name, AttrType::name};
    SingleCollection<C_job_save_disposition> job_save_disposition{
        AttrName::job_save_disposition};
    SingleValue<StringWithLanguage> job_sheet_message{
        AttrName::job_sheet_message, AttrType::text};
    SingleValue<E_job_sheets> job_sheets{AttrName::job_sheets,
                                         AttrType::keyword};
    SingleCollection<C_job_sheets_col> job_sheets_col{AttrName::job_sheets_col};
    SingleValue<E_media> media{AttrName::media, AttrType::keyword};
    SingleCollection<C_media_col> media_col{AttrName::media_col};
    SingleValue<E_media_input_tray_check> media_input_tray_check{
        AttrName::media_input_tray_check, AttrType::keyword};
    SingleValue<E_multiple_document_handling> multiple_document_handling{
        AttrName::multiple_document_handling, AttrType::keyword};
    SingleValue<int> number_up{AttrName::number_up, AttrType::integer};
    SingleValue<E_orientation_requested> orientation_requested{
        AttrName::orientation_requested, AttrType::enum_};
    SingleValue<E_output_bin> output_bin{AttrName::output_bin,
                                         AttrType::keyword};
    SingleValue<StringWithLanguage> output_device{AttrName::output_device,
                                                  AttrType::name};
    SetOfCollections<C_overrides> overrides{AttrName::overrides};
    SingleValue<E_page_delivery> page_delivery{AttrName::page_delivery,
                                               AttrType::keyword};
    SingleValue<E_page_order_received> page_order_received{
        AttrName::page_order_received, AttrType::keyword};
    SetOfValues<RangeOfInteger> page_ranges{AttrName::page_ranges,
                                            AttrType::rangeOfInteger};
    SetOfValues<int> pages_per_subset{AttrName::pages_per_subset,
                                      AttrType::integer};
    SetOfCollections<C_pdl_init_file> pdl_init_file{AttrName::pdl_init_file};
    SingleValue<E_presentation_direction_number_up>
        presentation_direction_number_up{
            AttrName::presentation_direction_number_up, AttrType::keyword};
    SingleValue<E_print_color_mode> print_color_mode{AttrName::print_color_mode,
                                                     AttrType::keyword};
    SingleValue<E_print_content_optimize> print_content_optimize{
        AttrName::print_content_optimize, AttrType::keyword};
    SingleValue<E_print_quality> print_quality{AttrName::print_quality,
                                               AttrType::enum_};
    SingleValue<E_print_rendering_intent> print_rendering_intent{
        AttrName::print_rendering_intent, AttrType::keyword};
    SingleValue<E_print_scaling> print_scaling{AttrName::print_scaling,
                                               AttrType::keyword};
    SingleValue<Resolution> printer_resolution{AttrName::printer_resolution,
                                               AttrType::resolution};
    SingleCollection<C_proof_print> proof_print{AttrName::proof_print};
    SingleCollection<C_separator_sheets> separator_sheets{
        AttrName::separator_sheets};
    SingleValue<E_sheet_collate> sheet_collate{AttrName::sheet_collate,
                                               AttrType::keyword};
    SingleValue<E_sides> sides{AttrName::sides, AttrType::keyword};
    SingleValue<E_x_image_position> x_image_position{AttrName::x_image_position,
                                                     AttrType::keyword};
    SingleValue<int> x_image_shift{AttrName::x_image_shift, AttrType::integer};
    SingleValue<int> x_side1_image_shift{AttrName::x_side1_image_shift,
                                         AttrType::integer};
    SingleValue<int> x_side2_image_shift{AttrName::x_side2_image_shift,
                                         AttrType::integer};
    SingleValue<E_y_image_position> y_image_position{AttrName::y_image_position,
                                                     AttrType::keyword};
    SingleValue<int> y_image_shift{AttrName::y_image_shift, AttrType::integer};
    SingleValue<int> y_side1_image_shift{AttrName::y_side1_image_shift,
                                         AttrType::integer};
    SingleValue<int> y_side2_image_shift{AttrName::y_side2_image_shift,
                                         AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_Create_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Create_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  struct G_job_attributes : public Collection {
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<E_job_state> job_state{AttrName::job_state, AttrType::enum_};
    SetOfValues<E_job_state_reasons> job_state_reasons{
        AttrName::job_state_reasons, AttrType::keyword};
    SingleValue<StringWithLanguage> job_state_message{
        AttrName::job_state_message, AttrType::text};
    SingleValue<int> number_of_intervening_jobs{
        AttrName::number_of_intervening_jobs, AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Create_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Get_Job_Attributes : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Get_Job_Attributes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Get_Job_Attributes : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  struct G_job_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<int> copies{AttrName::copies, AttrType::integer};
    SetOfValues<int> copies_actual{AttrName::copies_actual, AttrType::integer};
    SingleCollection<C_cover_back> cover_back{AttrName::cover_back};
    SetOfCollections<C_cover_back_actual> cover_back_actual{
        AttrName::cover_back_actual};
    SingleCollection<C_cover_front> cover_front{AttrName::cover_front};
    SetOfCollections<C_cover_front_actual> cover_front_actual{
        AttrName::cover_front_actual};
    SingleValue<E_current_page_order> current_page_order{
        AttrName::current_page_order, AttrType::keyword};
    SingleValue<DateTime> date_time_at_completed{
        AttrName::date_time_at_completed, AttrType::dateTime};
    SingleValue<DateTime> date_time_at_creation{AttrName::date_time_at_creation,
                                                AttrType::dateTime};
    SingleValue<DateTime> date_time_at_processing{
        AttrName::date_time_at_processing, AttrType::dateTime};
    SingleValue<std::string> document_charset_supplied{
        AttrName::document_charset_supplied, AttrType::charset};
    SetOfCollections<C_document_format_details_supplied>
        document_format_details_supplied{
            AttrName::document_format_details_supplied};
    SetOfValues<std::string> document_format_ready{
        AttrName::document_format_ready, AttrType::mimeMediaType};
    SingleValue<std::string> document_format_supplied{
        AttrName::document_format_supplied, AttrType::mimeMediaType};
    SingleValue<StringWithLanguage> document_format_version_supplied{
        AttrName::document_format_version_supplied, AttrType::text};
    SingleValue<StringWithLanguage> document_message_supplied{
        AttrName::document_message_supplied, AttrType::text};
    SetOfValues<std::string> document_metadata{AttrName::document_metadata,
                                               AttrType::octetString};
    SingleValue<StringWithLanguage> document_name_supplied{
        AttrName::document_name_supplied, AttrType::name};
    SingleValue<std::string> document_natural_language_supplied{
        AttrName::document_natural_language_supplied,
        AttrType::naturalLanguage};
    SingleValue<int> errors_count{AttrName::errors_count, AttrType::integer};
    SingleValue<E_feed_orientation> feed_orientation{AttrName::feed_orientation,
                                                     AttrType::keyword};
    SetOfValues<E_finishings> finishings{AttrName::finishings, AttrType::enum_};
    SetOfCollections<C_finishings_col> finishings_col{AttrName::finishings_col};
    SetOfCollections<C_finishings_col_actual> finishings_col_actual{
        AttrName::finishings_col_actual};
    SingleValue<StringWithLanguage> font_name_requested{
        AttrName::font_name_requested, AttrType::name};
    SingleValue<int> font_size_requested{AttrName::font_size_requested,
                                         AttrType::integer};
    SetOfValues<int> force_front_side{AttrName::force_front_side,
                                      AttrType::integer};
    SetOfValues<int> force_front_side_actual{AttrName::force_front_side_actual,
                                             AttrType::integer};
    SingleValue<E_imposition_template> imposition_template{
        AttrName::imposition_template, AttrType::keyword};
    SingleValue<int> impressions_completed_current_copy{
        AttrName::impressions_completed_current_copy, AttrType::integer};
    SetOfCollections<C_insert_sheet> insert_sheet{AttrName::insert_sheet};
    SetOfCollections<C_insert_sheet_actual> insert_sheet_actual{
        AttrName::insert_sheet_actual};
    SingleValue<StringWithLanguage> job_account_id{AttrName::job_account_id,
                                                   AttrType::name};
    SetOfValues<StringWithLanguage> job_account_id_actual{
        AttrName::job_account_id_actual, AttrType::name};
    SingleValue<E_job_account_type> job_account_type{AttrName::job_account_type,
                                                     AttrType::keyword};
    SingleCollection<C_job_accounting_sheets> job_accounting_sheets{
        AttrName::job_accounting_sheets};
    SetOfCollections<C_job_accounting_sheets_actual>
        job_accounting_sheets_actual{AttrName::job_accounting_sheets_actual};
    SingleValue<StringWithLanguage> job_accounting_user_id{
        AttrName::job_accounting_user_id, AttrType::name};
    SetOfValues<StringWithLanguage> job_accounting_user_id_actual{
        AttrName::job_accounting_user_id_actual, AttrType::name};
    SingleValue<bool> job_attribute_fidelity{AttrName::job_attribute_fidelity,
                                             AttrType::boolean};
    SingleValue<StringWithLanguage> job_charge_info{AttrName::job_charge_info,
                                                    AttrType::text};
    SingleValue<E_job_collation_type> job_collation_type{
        AttrName::job_collation_type, AttrType::enum_};
    SingleValue<int> job_copies{AttrName::job_copies, AttrType::integer};
    SetOfValues<int> job_copies_actual{AttrName::job_copies_actual,
                                       AttrType::integer};
    SingleCollection<C_job_cover_back> job_cover_back{AttrName::job_cover_back};
    SetOfCollections<C_job_cover_back_actual> job_cover_back_actual{
        AttrName::job_cover_back_actual};
    SingleCollection<C_job_cover_front> job_cover_front{
        AttrName::job_cover_front};
    SetOfCollections<C_job_cover_front_actual> job_cover_front_actual{
        AttrName::job_cover_front_actual};
    SingleValue<E_job_delay_output_until> job_delay_output_until{
        AttrName::job_delay_output_until, AttrType::keyword};
    SingleValue<DateTime> job_delay_output_until_time{
        AttrName::job_delay_output_until_time, AttrType::dateTime};
    SetOfValues<StringWithLanguage> job_detailed_status_messages{
        AttrName::job_detailed_status_messages, AttrType::text};
    SetOfValues<StringWithLanguage> job_document_access_errors{
        AttrName::job_document_access_errors, AttrType::text};
    SingleValue<E_job_error_action> job_error_action{AttrName::job_error_action,
                                                     AttrType::keyword};
    SingleCollection<C_job_error_sheet> job_error_sheet{
        AttrName::job_error_sheet};
    SetOfCollections<C_job_error_sheet_actual> job_error_sheet_actual{
        AttrName::job_error_sheet_actual};
    SetOfValues<E_job_finishings> job_finishings{AttrName::job_finishings,
                                                 AttrType::enum_};
    SetOfCollections<C_job_finishings_col> job_finishings_col{
        AttrName::job_finishings_col};
    SetOfCollections<C_job_finishings_col_actual> job_finishings_col_actual{
        AttrName::job_finishings_col_actual};
    SingleValue<E_job_hold_until> job_hold_until{AttrName::job_hold_until,
                                                 AttrType::keyword};
    SingleValue<DateTime> job_hold_until_time{AttrName::job_hold_until_time,
                                              AttrType::dateTime};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<int> job_impressions{AttrName::job_impressions,
                                     AttrType::integer};
    SingleValue<int> job_impressions_completed{
        AttrName::job_impressions_completed, AttrType::integer};
    SingleValue<int> job_k_octets{AttrName::job_k_octets, AttrType::integer};
    SingleValue<int> job_k_octets_processed{AttrName::job_k_octets_processed,
                                            AttrType::integer};
    SetOfValues<E_job_mandatory_attributes> job_mandatory_attributes{
        AttrName::job_mandatory_attributes, AttrType::keyword};
    SingleValue<int> job_media_sheets{AttrName::job_media_sheets,
                                      AttrType::integer};
    SingleValue<int> job_media_sheets_completed{
        AttrName::job_media_sheets_completed, AttrType::integer};
    SingleValue<StringWithLanguage> job_message_from_operator{
        AttrName::job_message_from_operator, AttrType::text};
    SingleValue<StringWithLanguage> job_message_to_operator{
        AttrName::job_message_to_operator, AttrType::text};
    SetOfValues<StringWithLanguage> job_message_to_operator_actual{
        AttrName::job_message_to_operator_actual, AttrType::text};
    SingleValue<std::string> job_more_info{AttrName::job_more_info,
                                           AttrType::uri};
    SingleValue<StringWithLanguage> job_name{AttrName::job_name,
                                             AttrType::name};
    SingleValue<StringWithLanguage> job_originating_user_name{
        AttrName::job_originating_user_name, AttrType::name};
    SingleValue<std::string> job_originating_user_uri{
        AttrName::job_originating_user_uri, AttrType::uri};
    SingleValue<int> job_pages{AttrName::job_pages, AttrType::integer};
    SingleValue<int> job_pages_completed{AttrName::job_pages_completed,
                                         AttrType::integer};
    SingleValue<int> job_pages_completed_current_copy{
        AttrName::job_pages_completed_current_copy, AttrType::integer};
    SingleValue<int> job_pages_per_set{AttrName::job_pages_per_set,
                                       AttrType::integer};
    SingleValue<std::string> job_phone_number{AttrName::job_phone_number,
                                              AttrType::uri};
    SingleValue<int> job_printer_up_time{AttrName::job_printer_up_time,
                                         AttrType::integer};
    SingleValue<std::string> job_printer_uri{AttrName::job_printer_uri,
                                             AttrType::uri};
    SingleValue<int> job_priority{AttrName::job_priority, AttrType::integer};
    SetOfValues<int> job_priority_actual{AttrName::job_priority_actual,
                                         AttrType::integer};
    SingleValue<StringWithLanguage> job_recipient_name{
        AttrName::job_recipient_name, AttrType::name};
    SetOfValues<int> job_resource_ids{AttrName::job_resource_ids,
                                      AttrType::integer};
    SingleCollection<C_job_save_disposition> job_save_disposition{
        AttrName::job_save_disposition};
    SingleValue<StringWithLanguage> job_save_printer_make_and_model{
        AttrName::job_save_printer_make_and_model, AttrType::text};
    SingleValue<StringWithLanguage> job_sheet_message{
        AttrName::job_sheet_message, AttrType::text};
    SetOfValues<StringWithLanguage> job_sheet_message_actual{
        AttrName::job_sheet_message_actual, AttrType::text};
    SingleValue<E_job_sheets> job_sheets{AttrName::job_sheets,
                                         AttrType::keyword};
    SingleCollection<C_job_sheets_col> job_sheets_col{AttrName::job_sheets_col};
    SetOfCollections<C_job_sheets_col_actual> job_sheets_col_actual{
        AttrName::job_sheets_col_actual};
    SingleValue<E_job_state> job_state{AttrName::job_state, AttrType::enum_};
    SingleValue<StringWithLanguage> job_state_message{
        AttrName::job_state_message, AttrType::text};
    SetOfValues<E_job_state_reasons> job_state_reasons{
        AttrName::job_state_reasons, AttrType::keyword};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<std::string> job_uuid{AttrName::job_uuid, AttrType::uri};
    SingleValue<E_media> media{AttrName::media, AttrType::keyword};
    SingleCollection<C_media_col> media_col{AttrName::media_col};
    SetOfCollections<C_media_col_actual> media_col_actual{
        AttrName::media_col_actual};
    SingleValue<E_media_input_tray_check> media_input_tray_check{
        AttrName::media_input_tray_check, AttrType::keyword};
    SingleValue<E_multiple_document_handling> multiple_document_handling{
        AttrName::multiple_document_handling, AttrType::keyword};
    SingleValue<int> number_of_documents{AttrName::number_of_documents,
                                         AttrType::integer};
    SingleValue<int> number_of_intervening_jobs{
        AttrName::number_of_intervening_jobs, AttrType::integer};
    SingleValue<int> number_up{AttrName::number_up, AttrType::integer};
    SetOfValues<int> number_up_actual{AttrName::number_up_actual,
                                      AttrType::integer};
    SingleValue<E_orientation_requested> orientation_requested{
        AttrName::orientation_requested, AttrType::enum_};
    SingleValue<StringWithLanguage> original_requesting_user_name{
        AttrName::original_requesting_user_name, AttrType::name};
    SingleValue<E_output_bin> output_bin{AttrName::output_bin,
                                         AttrType::keyword};
    SingleValue<StringWithLanguage> output_device{AttrName::output_device,
                                                  AttrType::name};
    SetOfValues<StringWithLanguage> output_device_actual{
        AttrName::output_device_actual, AttrType::name};
    SingleValue<StringWithLanguage> output_device_assigned{
        AttrName::output_device_assigned, AttrType::name};
    SingleValue<StringWithLanguage> output_device_job_state_message{
        AttrName::output_device_job_state_message, AttrType::text};
    SingleValue<std::string> output_device_uuid_assigned{
        AttrName::output_device_uuid_assigned, AttrType::uri};
    SetOfCollections<C_overrides> overrides{AttrName::overrides};
    SetOfCollections<C_overrides_actual> overrides_actual{
        AttrName::overrides_actual};
    SingleValue<E_page_delivery> page_delivery{AttrName::page_delivery,
                                               AttrType::keyword};
    SingleValue<E_page_order_received> page_order_received{
        AttrName::page_order_received, AttrType::keyword};
    SetOfValues<RangeOfInteger> page_ranges{AttrName::page_ranges,
                                            AttrType::rangeOfInteger};
    SetOfValues<RangeOfInteger> page_ranges_actual{AttrName::page_ranges_actual,
                                                   AttrType::rangeOfInteger};
    SetOfValues<int> pages_per_subset{AttrName::pages_per_subset,
                                      AttrType::integer};
    SetOfCollections<C_pdl_init_file> pdl_init_file{AttrName::pdl_init_file};
    SingleValue<E_presentation_direction_number_up>
        presentation_direction_number_up{
            AttrName::presentation_direction_number_up, AttrType::keyword};
    SingleValue<E_print_color_mode> print_color_mode{AttrName::print_color_mode,
                                                     AttrType::keyword};
    SingleValue<E_print_content_optimize> print_content_optimize{
        AttrName::print_content_optimize, AttrType::keyword};
    SingleValue<E_print_quality> print_quality{AttrName::print_quality,
                                               AttrType::enum_};
    SingleValue<E_print_rendering_intent> print_rendering_intent{
        AttrName::print_rendering_intent, AttrType::keyword};
    SingleValue<E_print_scaling> print_scaling{AttrName::print_scaling,
                                               AttrType::keyword};
    SingleValue<Resolution> printer_resolution{AttrName::printer_resolution,
                                               AttrType::resolution};
    SetOfValues<Resolution> printer_resolution_actual{
        AttrName::printer_resolution_actual, AttrType::resolution};
    SingleCollection<C_proof_print> proof_print{AttrName::proof_print};
    SingleCollection<C_separator_sheets> separator_sheets{
        AttrName::separator_sheets};
    SetOfCollections<C_separator_sheets_actual> separator_sheets_actual{
        AttrName::separator_sheets_actual};
    SingleValue<E_sheet_collate> sheet_collate{AttrName::sheet_collate,
                                               AttrType::keyword};
    SingleValue<int> sheet_completed_copy_number{
        AttrName::sheet_completed_copy_number, AttrType::integer};
    SingleValue<int> sheet_completed_document_number{
        AttrName::sheet_completed_document_number, AttrType::integer};
    SingleValue<E_sides> sides{AttrName::sides, AttrType::keyword};
    SingleValue<int> time_at_completed{AttrName::time_at_completed,
                                       AttrType::integer};
    SingleValue<int> time_at_creation{AttrName::time_at_creation,
                                      AttrType::integer};
    SingleValue<int> time_at_processing{AttrName::time_at_processing,
                                        AttrType::integer};
    SingleValue<int> warnings_count{AttrName::warnings_count,
                                    AttrType::integer};
    SingleValue<E_x_image_position> x_image_position{AttrName::x_image_position,
                                                     AttrType::keyword};
    SingleValue<int> x_image_shift{AttrName::x_image_shift, AttrType::integer};
    SetOfValues<int> x_image_shift_actual{AttrName::x_image_shift_actual,
                                          AttrType::integer};
    SingleValue<int> x_side1_image_shift{AttrName::x_side1_image_shift,
                                         AttrType::integer};
    SetOfValues<int> x_side1_image_shift_actual{
        AttrName::x_side1_image_shift_actual, AttrType::integer};
    SingleValue<int> x_side2_image_shift{AttrName::x_side2_image_shift,
                                         AttrType::integer};
    SetOfValues<int> x_side2_image_shift_actual{
        AttrName::x_side2_image_shift_actual, AttrType::integer};
    SingleValue<E_y_image_position> y_image_position{AttrName::y_image_position,
                                                     AttrType::keyword};
    SingleValue<int> y_image_shift{AttrName::y_image_shift, AttrType::integer};
    SetOfValues<int> y_image_shift_actual{AttrName::y_image_shift_actual,
                                          AttrType::integer};
    SingleValue<int> y_side1_image_shift{AttrName::y_side1_image_shift,
                                         AttrType::integer};
    SetOfValues<int> y_side1_image_shift_actual{
        AttrName::y_side1_image_shift_actual, AttrType::integer};
    SingleValue<int> y_side2_image_shift{AttrName::y_side2_image_shift,
                                         AttrType::integer};
    SetOfValues<int> y_side2_image_shift_actual{
        AttrName::y_side2_image_shift_actual, AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Get_Job_Attributes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Get_Jobs : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<int> limit{AttrName::limit, AttrType::integer};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    SingleValue<E_which_jobs> which_jobs{AttrName::which_jobs,
                                         AttrType::keyword};
    SingleValue<bool> my_jobs{AttrName::my_jobs, AttrType::boolean};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Get_Jobs();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Get_Jobs : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  typedef Response_Get_Job_Attributes::G_job_attributes G_job_attributes;
  SetOfGroups<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Get_Jobs();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Get_Printer_Attributes : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    OpenSetOfValues<E_requested_attributes> requested_attributes{
        AttrName::requested_attributes, AttrType::keyword};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Get_Printer_Attributes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Get_Printer_Attributes : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  struct G_printer_attributes : public Collection {
    OpenSetOfValues<E_baling_type_supported> baling_type_supported{
        AttrName::baling_type_supported, AttrType::keyword};
    SetOfValues<E_baling_when_supported> baling_when_supported{
        AttrName::baling_when_supported, AttrType::keyword};
    SetOfValues<E_binding_reference_edge_supported>
        binding_reference_edge_supported{
            AttrName::binding_reference_edge_supported, AttrType::keyword};
    SetOfValues<E_binding_type_supported> binding_type_supported{
        AttrName::binding_type_supported, AttrType::keyword};
    SingleValue<std::string> charset_configured{AttrName::charset_configured,
                                                AttrType::charset};
    SetOfValues<std::string> charset_supported{AttrName::charset_supported,
                                               AttrType::charset};
    SetOfValues<E_coating_sides_supported> coating_sides_supported{
        AttrName::coating_sides_supported, AttrType::keyword};
    OpenSetOfValues<E_coating_type_supported> coating_type_supported{
        AttrName::coating_type_supported, AttrType::keyword};
    SingleValue<bool> color_supported{AttrName::color_supported,
                                      AttrType::boolean};
    SetOfValues<E_compression_supported> compression_supported{
        AttrName::compression_supported, AttrType::keyword};
    SingleValue<int> copies_default{AttrName::copies_default,
                                    AttrType::integer};
    SingleValue<RangeOfInteger> copies_supported{AttrName::copies_supported,
                                                 AttrType::rangeOfInteger};
    SingleCollection<C_cover_back_default> cover_back_default{
        AttrName::cover_back_default};
    SetOfValues<E_cover_back_supported> cover_back_supported{
        AttrName::cover_back_supported, AttrType::keyword};
    SingleCollection<C_cover_front_default> cover_front_default{
        AttrName::cover_front_default};
    SetOfValues<E_cover_front_supported> cover_front_supported{
        AttrName::cover_front_supported, AttrType::keyword};
    OpenSetOfValues<E_covering_name_supported> covering_name_supported{
        AttrName::covering_name_supported, AttrType::keyword};
    SingleValue<int> device_service_count{AttrName::device_service_count,
                                          AttrType::integer};
    SingleValue<std::string> device_uuid{AttrName::device_uuid, AttrType::uri};
    SingleValue<std::string> document_charset_default{
        AttrName::document_charset_default, AttrType::charset};
    SetOfValues<std::string> document_charset_supported{
        AttrName::document_charset_supported, AttrType::charset};
    SingleValue<E_document_digital_signature_default>
        document_digital_signature_default{
            AttrName::document_digital_signature_default, AttrType::keyword};
    SetOfValues<E_document_digital_signature_supported>
        document_digital_signature_supported{
            AttrName::document_digital_signature_supported, AttrType::keyword};
    SingleValue<std::string> document_format_default{
        AttrName::document_format_default, AttrType::mimeMediaType};
    SingleCollection<C_document_format_details_default>
        document_format_details_default{
            AttrName::document_format_details_default};
    SetOfValues<E_document_format_details_supported>
        document_format_details_supported{
            AttrName::document_format_details_supported, AttrType::keyword};
    SetOfValues<std::string> document_format_supported{
        AttrName::document_format_supported, AttrType::mimeMediaType};
    SetOfValues<E_document_format_varying_attributes>
        document_format_varying_attributes{
            AttrName::document_format_varying_attributes, AttrType::keyword};
    SingleValue<StringWithLanguage> document_format_version_default{
        AttrName::document_format_version_default, AttrType::text};
    SetOfValues<StringWithLanguage> document_format_version_supported{
        AttrName::document_format_version_supported, AttrType::text};
    SingleValue<std::string> document_natural_language_default{
        AttrName::document_natural_language_default, AttrType::naturalLanguage};
    SetOfValues<std::string> document_natural_language_supported{
        AttrName::document_natural_language_supported,
        AttrType::naturalLanguage};
    SingleValue<int> document_password_supported{
        AttrName::document_password_supported, AttrType::integer};
    SetOfValues<E_feed_orientation_supported> feed_orientation_supported{
        AttrName::feed_orientation_supported, AttrType::keyword};
    OpenSetOfValues<E_finishing_template_supported>
        finishing_template_supported{AttrName::finishing_template_supported,
                                     AttrType::keyword};
    SetOfCollections<C_finishings_col_database> finishings_col_database{
        AttrName::finishings_col_database};
    SetOfCollections<C_finishings_col_default> finishings_col_default{
        AttrName::finishings_col_default};
    SetOfCollections<C_finishings_col_ready> finishings_col_ready{
        AttrName::finishings_col_ready};
    SetOfValues<E_finishings_default> finishings_default{
        AttrName::finishings_default, AttrType::enum_};
    SetOfValues<E_finishings_ready> finishings_ready{AttrName::finishings_ready,
                                                     AttrType::enum_};
    SetOfValues<E_finishings_supported> finishings_supported{
        AttrName::finishings_supported, AttrType::enum_};
    SetOfValues<E_folding_direction_supported> folding_direction_supported{
        AttrName::folding_direction_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> folding_offset_supported{
        AttrName::folding_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_folding_reference_edge_supported>
        folding_reference_edge_supported{
            AttrName::folding_reference_edge_supported, AttrType::keyword};
    SingleValue<StringWithLanguage> font_name_requested_default{
        AttrName::font_name_requested_default, AttrType::name};
    SetOfValues<StringWithLanguage> font_name_requested_supported{
        AttrName::font_name_requested_supported, AttrType::name};
    SingleValue<int> font_size_requested_default{
        AttrName::font_size_requested_default, AttrType::integer};
    SetOfValues<RangeOfInteger> font_size_requested_supported{
        AttrName::font_size_requested_supported, AttrType::rangeOfInteger};
    SetOfValues<std::string> generated_natural_language_supported{
        AttrName::generated_natural_language_supported,
        AttrType::naturalLanguage};
    SetOfValues<E_identify_actions_default> identify_actions_default{
        AttrName::identify_actions_default, AttrType::keyword};
    SetOfValues<E_identify_actions_supported> identify_actions_supported{
        AttrName::identify_actions_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> insert_after_page_number_supported{
        AttrName::insert_after_page_number_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> insert_count_supported{
        AttrName::insert_count_supported, AttrType::rangeOfInteger};
    SetOfCollections<C_insert_sheet_default> insert_sheet_default{
        AttrName::insert_sheet_default};
    SetOfValues<E_ipp_features_supported> ipp_features_supported{
        AttrName::ipp_features_supported, AttrType::keyword};
    SetOfValues<E_ipp_versions_supported> ipp_versions_supported{
        AttrName::ipp_versions_supported, AttrType::keyword};
    SingleValue<int> ippget_event_life{AttrName::ippget_event_life,
                                       AttrType::integer};
    SingleValue<StringWithLanguage> job_account_id_default{
        AttrName::job_account_id_default, AttrType::name};
    SingleValue<bool> job_account_id_supported{
        AttrName::job_account_id_supported, AttrType::boolean};
    SingleValue<E_job_account_type_default> job_account_type_default{
        AttrName::job_account_type_default, AttrType::keyword};
    OpenSetOfValues<E_job_account_type_supported> job_account_type_supported{
        AttrName::job_account_type_supported, AttrType::keyword};
    SingleCollection<C_job_accounting_sheets_default>
        job_accounting_sheets_default{AttrName::job_accounting_sheets_default};
    SingleValue<StringWithLanguage> job_accounting_user_id_default{
        AttrName::job_accounting_user_id_default, AttrType::name};
    SingleValue<bool> job_accounting_user_id_supported{
        AttrName::job_accounting_user_id_supported, AttrType::boolean};
    SingleValue<bool> job_authorization_uri_supported{
        AttrName::job_authorization_uri_supported, AttrType::boolean};
    SetOfCollections<C_job_constraints_supported> job_constraints_supported{
        AttrName::job_constraints_supported};
    SingleValue<int> job_copies_default{AttrName::job_copies_default,
                                        AttrType::integer};
    SingleValue<RangeOfInteger> job_copies_supported{
        AttrName::job_copies_supported, AttrType::rangeOfInteger};
    SingleCollection<C_job_cover_back_default> job_cover_back_default{
        AttrName::job_cover_back_default};
    SetOfValues<E_job_cover_back_supported> job_cover_back_supported{
        AttrName::job_cover_back_supported, AttrType::keyword};
    SingleCollection<C_job_cover_front_default> job_cover_front_default{
        AttrName::job_cover_front_default};
    SetOfValues<E_job_cover_front_supported> job_cover_front_supported{
        AttrName::job_cover_front_supported, AttrType::keyword};
    SingleValue<E_job_delay_output_until_default>
        job_delay_output_until_default{AttrName::job_delay_output_until_default,
                                       AttrType::keyword};
    OpenSetOfValues<E_job_delay_output_until_supported>
        job_delay_output_until_supported{
            AttrName::job_delay_output_until_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> job_delay_output_until_time_supported{
        AttrName::job_delay_output_until_time_supported,
        AttrType::rangeOfInteger};
    SingleValue<E_job_error_action_default> job_error_action_default{
        AttrName::job_error_action_default, AttrType::keyword};
    SetOfValues<E_job_error_action_supported> job_error_action_supported{
        AttrName::job_error_action_supported, AttrType::keyword};
    SingleCollection<C_job_error_sheet_default> job_error_sheet_default{
        AttrName::job_error_sheet_default};
    SetOfCollections<C_job_finishings_col_default> job_finishings_col_default{
        AttrName::job_finishings_col_default};
    SetOfCollections<C_job_finishings_col_ready> job_finishings_col_ready{
        AttrName::job_finishings_col_ready};
    SetOfValues<E_job_finishings_default> job_finishings_default{
        AttrName::job_finishings_default, AttrType::enum_};
    SetOfValues<E_job_finishings_ready> job_finishings_ready{
        AttrName::job_finishings_ready, AttrType::enum_};
    SetOfValues<E_job_finishings_supported> job_finishings_supported{
        AttrName::job_finishings_supported, AttrType::enum_};
    SingleValue<E_job_hold_until_default> job_hold_until_default{
        AttrName::job_hold_until_default, AttrType::keyword};
    OpenSetOfValues<E_job_hold_until_supported> job_hold_until_supported{
        AttrName::job_hold_until_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> job_hold_until_time_supported{
        AttrName::job_hold_until_time_supported, AttrType::rangeOfInteger};
    SingleValue<bool> job_ids_supported{AttrName::job_ids_supported,
                                        AttrType::boolean};
    SingleValue<RangeOfInteger> job_impressions_supported{
        AttrName::job_impressions_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> job_k_octets_supported{
        AttrName::job_k_octets_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> job_media_sheets_supported{
        AttrName::job_media_sheets_supported, AttrType::rangeOfInteger};
    SingleValue<StringWithLanguage> job_message_to_operator_default{
        AttrName::job_message_to_operator_default, AttrType::text};
    SingleValue<bool> job_message_to_operator_supported{
        AttrName::job_message_to_operator_supported, AttrType::boolean};
    SingleValue<bool> job_pages_per_set_supported{
        AttrName::job_pages_per_set_supported, AttrType::boolean};
    OpenSetOfValues<E_job_password_encryption_supported>
        job_password_encryption_supported{
            AttrName::job_password_encryption_supported, AttrType::keyword};
    SingleValue<int> job_password_supported{AttrName::job_password_supported,
                                            AttrType::integer};
    SingleValue<std::string> job_phone_number_default{
        AttrName::job_phone_number_default, AttrType::uri};
    SingleValue<bool> job_phone_number_supported{
        AttrName::job_phone_number_supported, AttrType::boolean};
    SingleValue<int> job_priority_default{AttrName::job_priority_default,
                                          AttrType::integer};
    SingleValue<int> job_priority_supported{AttrName::job_priority_supported,
                                            AttrType::integer};
    SingleValue<StringWithLanguage> job_recipient_name_default{
        AttrName::job_recipient_name_default, AttrType::name};
    SingleValue<bool> job_recipient_name_supported{
        AttrName::job_recipient_name_supported, AttrType::boolean};
    SetOfCollections<C_job_resolvers_supported> job_resolvers_supported{
        AttrName::job_resolvers_supported};
    SingleValue<StringWithLanguage> job_sheet_message_default{
        AttrName::job_sheet_message_default, AttrType::text};
    SingleValue<bool> job_sheet_message_supported{
        AttrName::job_sheet_message_supported, AttrType::boolean};
    SingleCollection<C_job_sheets_col_default> job_sheets_col_default{
        AttrName::job_sheets_col_default};
    SingleValue<E_job_sheets_default> job_sheets_default{
        AttrName::job_sheets_default, AttrType::keyword};
    OpenSetOfValues<E_job_sheets_supported> job_sheets_supported{
        AttrName::job_sheets_supported, AttrType::keyword};
    SingleValue<E_job_spooling_supported> job_spooling_supported{
        AttrName::job_spooling_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> jpeg_k_octets_supported{
        AttrName::jpeg_k_octets_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> jpeg_x_dimension_supported{
        AttrName::jpeg_x_dimension_supported, AttrType::rangeOfInteger};
    SingleValue<RangeOfInteger> jpeg_y_dimension_supported{
        AttrName::jpeg_y_dimension_supported, AttrType::rangeOfInteger};
    SetOfValues<E_laminating_sides_supported> laminating_sides_supported{
        AttrName::laminating_sides_supported, AttrType::keyword};
    OpenSetOfValues<E_laminating_type_supported> laminating_type_supported{
        AttrName::laminating_type_supported, AttrType::keyword};
    SingleValue<int> max_save_info_supported{AttrName::max_save_info_supported,
                                             AttrType::integer};
    SingleValue<int> max_stitching_locations_supported{
        AttrName::max_stitching_locations_supported, AttrType::integer};
    OpenSetOfValues<E_media_back_coating_supported>
        media_back_coating_supported{AttrName::media_back_coating_supported,
                                     AttrType::keyword};
    SetOfValues<int> media_bottom_margin_supported{
        AttrName::media_bottom_margin_supported, AttrType::integer};
    SetOfCollections<C_media_col_database> media_col_database{
        AttrName::media_col_database};
    SingleCollection<C_media_col_default> media_col_default{
        AttrName::media_col_default};
    SetOfCollections<C_media_col_ready> media_col_ready{
        AttrName::media_col_ready};
    OpenSetOfValues<E_media_color_supported> media_color_supported{
        AttrName::media_color_supported, AttrType::keyword};
    SingleValue<E_media_default> media_default{AttrName::media_default,
                                               AttrType::keyword};
    OpenSetOfValues<E_media_front_coating_supported>
        media_front_coating_supported{AttrName::media_front_coating_supported,
                                      AttrType::keyword};
    OpenSetOfValues<E_media_grain_supported> media_grain_supported{
        AttrName::media_grain_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> media_hole_count_supported{
        AttrName::media_hole_count_supported, AttrType::rangeOfInteger};
    SingleValue<bool> media_info_supported{AttrName::media_info_supported,
                                           AttrType::boolean};
    SetOfValues<int> media_left_margin_supported{
        AttrName::media_left_margin_supported, AttrType::integer};
    SetOfValues<RangeOfInteger> media_order_count_supported{
        AttrName::media_order_count_supported, AttrType::rangeOfInteger};
    OpenSetOfValues<E_media_pre_printed_supported> media_pre_printed_supported{
        AttrName::media_pre_printed_supported, AttrType::keyword};
    OpenSetOfValues<E_media_ready> media_ready{AttrName::media_ready,
                                               AttrType::keyword};
    OpenSetOfValues<E_media_recycled_supported> media_recycled_supported{
        AttrName::media_recycled_supported, AttrType::keyword};
    SetOfValues<int> media_right_margin_supported{
        AttrName::media_right_margin_supported, AttrType::integer};
    SetOfCollections<C_media_size_supported> media_size_supported{
        AttrName::media_size_supported};
    OpenSetOfValues<E_media_source_supported> media_source_supported{
        AttrName::media_source_supported, AttrType::keyword};
    OpenSetOfValues<E_media_supported> media_supported{
        AttrName::media_supported, AttrType::keyword};
    SingleValue<RangeOfInteger> media_thickness_supported{
        AttrName::media_thickness_supported, AttrType::rangeOfInteger};
    OpenSetOfValues<E_media_tooth_supported> media_tooth_supported{
        AttrName::media_tooth_supported, AttrType::keyword};
    SetOfValues<int> media_top_margin_supported{
        AttrName::media_top_margin_supported, AttrType::integer};
    OpenSetOfValues<E_media_type_supported> media_type_supported{
        AttrName::media_type_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> media_weight_metric_supported{
        AttrName::media_weight_metric_supported, AttrType::rangeOfInteger};
    SingleValue<E_multiple_document_handling_default>
        multiple_document_handling_default{
            AttrName::multiple_document_handling_default, AttrType::keyword};
    SetOfValues<E_multiple_document_handling_supported>
        multiple_document_handling_supported{
            AttrName::multiple_document_handling_supported, AttrType::keyword};
    SingleValue<bool> multiple_document_jobs_supported{
        AttrName::multiple_document_jobs_supported, AttrType::boolean};
    SingleValue<int> multiple_operation_time_out{
        AttrName::multiple_operation_time_out, AttrType::integer};
    SingleValue<E_multiple_operation_time_out_action>
        multiple_operation_time_out_action{
            AttrName::multiple_operation_time_out_action, AttrType::keyword};
    SingleValue<std::string> natural_language_configured{
        AttrName::natural_language_configured, AttrType::naturalLanguage};
    SetOfValues<E_notify_events_default> notify_events_default{
        AttrName::notify_events_default, AttrType::keyword};
    SetOfValues<E_notify_events_supported> notify_events_supported{
        AttrName::notify_events_supported, AttrType::keyword};
    SingleValue<int> notify_lease_duration_default{
        AttrName::notify_lease_duration_default, AttrType::integer};
    SetOfValues<RangeOfInteger> notify_lease_duration_supported{
        AttrName::notify_lease_duration_supported, AttrType::rangeOfInteger};
    SetOfValues<E_notify_pull_method_supported> notify_pull_method_supported{
        AttrName::notify_pull_method_supported, AttrType::keyword};
    SetOfValues<std::string> notify_schemes_supported{
        AttrName::notify_schemes_supported, AttrType::uriScheme};
    SingleValue<int> number_up_default{AttrName::number_up_default,
                                       AttrType::integer};
    SingleValue<RangeOfInteger> number_up_supported{
        AttrName::number_up_supported, AttrType::rangeOfInteger};
    SingleValue<std::string> oauth_authorization_server_uri{
        AttrName::oauth_authorization_server_uri, AttrType::uri};
    SetOfValues<E_operations_supported> operations_supported{
        AttrName::operations_supported, AttrType::enum_};
    SingleValue<E_orientation_requested_default> orientation_requested_default{
        AttrName::orientation_requested_default, AttrType::enum_};
    SetOfValues<E_orientation_requested_supported>
        orientation_requested_supported{
            AttrName::orientation_requested_supported, AttrType::enum_};
    SingleValue<E_output_bin_default> output_bin_default{
        AttrName::output_bin_default, AttrType::keyword};
    OpenSetOfValues<E_output_bin_supported> output_bin_supported{
        AttrName::output_bin_supported, AttrType::keyword};
    SetOfValues<StringWithLanguage> output_device_supported{
        AttrName::output_device_supported, AttrType::name};
    SetOfValues<std::string> output_device_uuid_supported{
        AttrName::output_device_uuid_supported, AttrType::uri};
    SingleValue<E_page_delivery_default> page_delivery_default{
        AttrName::page_delivery_default, AttrType::keyword};
    SetOfValues<E_page_delivery_supported> page_delivery_supported{
        AttrName::page_delivery_supported, AttrType::keyword};
    SingleValue<E_page_order_received_default> page_order_received_default{
        AttrName::page_order_received_default, AttrType::keyword};
    SetOfValues<E_page_order_received_supported> page_order_received_supported{
        AttrName::page_order_received_supported, AttrType::keyword};
    SingleValue<bool> page_ranges_supported{AttrName::page_ranges_supported,
                                            AttrType::boolean};
    SingleValue<int> pages_per_minute{AttrName::pages_per_minute,
                                      AttrType::integer};
    SingleValue<int> pages_per_minute_color{AttrName::pages_per_minute_color,
                                            AttrType::integer};
    SingleValue<bool> pages_per_subset_supported{
        AttrName::pages_per_subset_supported, AttrType::boolean};
    SetOfValues<std::string> parent_printers_supported{
        AttrName::parent_printers_supported, AttrType::uri};
    SingleValue<RangeOfInteger> pdf_k_octets_supported{
        AttrName::pdf_k_octets_supported, AttrType::rangeOfInteger};
    SetOfValues<E_pdf_versions_supported> pdf_versions_supported{
        AttrName::pdf_versions_supported, AttrType::keyword};
    SingleCollection<C_pdl_init_file_default> pdl_init_file_default{
        AttrName::pdl_init_file_default};
    SetOfValues<StringWithLanguage> pdl_init_file_entry_supported{
        AttrName::pdl_init_file_entry_supported, AttrType::name};
    SetOfValues<std::string> pdl_init_file_location_supported{
        AttrName::pdl_init_file_location_supported, AttrType::uri};
    SingleValue<bool> pdl_init_file_name_subdirectory_supported{
        AttrName::pdl_init_file_name_subdirectory_supported, AttrType::boolean};
    SetOfValues<StringWithLanguage> pdl_init_file_name_supported{
        AttrName::pdl_init_file_name_supported, AttrType::name};
    SetOfValues<E_pdl_init_file_supported> pdl_init_file_supported{
        AttrName::pdl_init_file_supported, AttrType::keyword};
    SingleValue<E_pdl_override_supported> pdl_override_supported{
        AttrName::pdl_override_supported, AttrType::keyword};
    SingleValue<bool> preferred_attributes_supported{
        AttrName::preferred_attributes_supported, AttrType::boolean};
    SingleValue<E_presentation_direction_number_up_default>
        presentation_direction_number_up_default{
            AttrName::presentation_direction_number_up_default,
            AttrType::keyword};
    SetOfValues<E_presentation_direction_number_up_supported>
        presentation_direction_number_up_supported{
            AttrName::presentation_direction_number_up_supported,
            AttrType::keyword};
    SingleValue<E_print_color_mode_default> print_color_mode_default{
        AttrName::print_color_mode_default, AttrType::keyword};
    SetOfValues<E_print_color_mode_supported> print_color_mode_supported{
        AttrName::print_color_mode_supported, AttrType::keyword};
    SingleValue<E_print_content_optimize_default>
        print_content_optimize_default{AttrName::print_content_optimize_default,
                                       AttrType::keyword};
    SetOfValues<E_print_content_optimize_supported>
        print_content_optimize_supported{
            AttrName::print_content_optimize_supported, AttrType::keyword};
    SingleValue<E_print_quality_default> print_quality_default{
        AttrName::print_quality_default, AttrType::enum_};
    SetOfValues<E_print_quality_supported> print_quality_supported{
        AttrName::print_quality_supported, AttrType::enum_};
    SingleValue<E_print_rendering_intent_default>
        print_rendering_intent_default{AttrName::print_rendering_intent_default,
                                       AttrType::keyword};
    SetOfValues<E_print_rendering_intent_supported>
        print_rendering_intent_supported{
            AttrName::print_rendering_intent_supported, AttrType::keyword};
    SetOfValues<std::string> printer_alert{AttrName::printer_alert,
                                           AttrType::octetString};
    SetOfValues<StringWithLanguage> printer_alert_description{
        AttrName::printer_alert_description, AttrType::text};
    SingleValue<StringWithLanguage> printer_charge_info{
        AttrName::printer_charge_info, AttrType::text};
    SingleValue<std::string> printer_charge_info_uri{
        AttrName::printer_charge_info_uri, AttrType::uri};
    SingleValue<DateTime> printer_config_change_date_time{
        AttrName::printer_config_change_date_time, AttrType::dateTime};
    SingleValue<int> printer_config_change_time{
        AttrName::printer_config_change_time, AttrType::integer};
    SingleValue<int> printer_config_changes{AttrName::printer_config_changes,
                                            AttrType::integer};
    SingleCollection<C_printer_contact_col> printer_contact_col{
        AttrName::printer_contact_col};
    SingleValue<DateTime> printer_current_time{AttrName::printer_current_time,
                                               AttrType::dateTime};
    SetOfValues<StringWithLanguage> printer_detailed_status_messages{
        AttrName::printer_detailed_status_messages, AttrType::text};
    SingleValue<StringWithLanguage> printer_device_id{
        AttrName::printer_device_id, AttrType::text};
    SingleValue<StringWithLanguage> printer_dns_sd_name{
        AttrName::printer_dns_sd_name, AttrType::name};
    SingleValue<std::string> printer_driver_installer{
        AttrName::printer_driver_installer, AttrType::uri};
    SetOfValues<std::string> printer_finisher{AttrName::printer_finisher,
                                              AttrType::octetString};
    SetOfValues<StringWithLanguage> printer_finisher_description{
        AttrName::printer_finisher_description, AttrType::text};
    SetOfValues<std::string> printer_finisher_supplies{
        AttrName::printer_finisher_supplies, AttrType::octetString};
    SetOfValues<StringWithLanguage> printer_finisher_supplies_description{
        AttrName::printer_finisher_supplies_description, AttrType::text};
    SingleValue<std::string> printer_geo_location{
        AttrName::printer_geo_location, AttrType::uri};
    SetOfCollections<C_printer_icc_profiles> printer_icc_profiles{
        AttrName::printer_icc_profiles};
    SetOfValues<std::string> printer_icons{AttrName::printer_icons,
                                           AttrType::uri};
    SingleValue<int> printer_id{AttrName::printer_id, AttrType::integer};
    SingleValue<int> printer_impressions_completed{
        AttrName::printer_impressions_completed, AttrType::integer};
    SingleValue<StringWithLanguage> printer_info{AttrName::printer_info,
                                                 AttrType::text};
    SetOfValues<std::string> printer_input_tray{AttrName::printer_input_tray,
                                                AttrType::octetString};
    SingleValue<bool> printer_is_accepting_jobs{
        AttrName::printer_is_accepting_jobs, AttrType::boolean};
    SingleValue<StringWithLanguage> printer_location{AttrName::printer_location,
                                                     AttrType::text};
    SingleValue<StringWithLanguage> printer_make_and_model{
        AttrName::printer_make_and_model, AttrType::text};
    SingleValue<int> printer_media_sheets_completed{
        AttrName::printer_media_sheets_completed, AttrType::integer};
    SingleValue<DateTime> printer_message_date_time{
        AttrName::printer_message_date_time, AttrType::dateTime};
    SingleValue<StringWithLanguage> printer_message_from_operator{
        AttrName::printer_message_from_operator, AttrType::text};
    SingleValue<int> printer_message_time{AttrName::printer_message_time,
                                          AttrType::integer};
    SingleValue<std::string> printer_more_info{AttrName::printer_more_info,
                                               AttrType::uri};
    SingleValue<std::string> printer_more_info_manufacturer{
        AttrName::printer_more_info_manufacturer, AttrType::uri};
    SingleValue<StringWithLanguage> printer_name{AttrName::printer_name,
                                                 AttrType::name};
    SetOfValues<StringWithLanguage> printer_organization{
        AttrName::printer_organization, AttrType::text};
    SetOfValues<StringWithLanguage> printer_organizational_unit{
        AttrName::printer_organizational_unit, AttrType::text};
    SetOfValues<std::string> printer_output_tray{AttrName::printer_output_tray,
                                                 AttrType::octetString};
    SingleValue<int> printer_pages_completed{AttrName::printer_pages_completed,
                                             AttrType::integer};
    SingleValue<Resolution> printer_resolution_default{
        AttrName::printer_resolution_default, AttrType::resolution};
    SingleValue<Resolution> printer_resolution_supported{
        AttrName::printer_resolution_supported, AttrType::resolution};
    SingleValue<E_printer_state> printer_state{AttrName::printer_state,
                                               AttrType::enum_};
    SingleValue<DateTime> printer_state_change_date_time{
        AttrName::printer_state_change_date_time, AttrType::dateTime};
    SingleValue<int> printer_state_change_time{
        AttrName::printer_state_change_time, AttrType::integer};
    SingleValue<StringWithLanguage> printer_state_message{
        AttrName::printer_state_message, AttrType::text};
    SetOfValues<E_printer_state_reasons> printer_state_reasons{
        AttrName::printer_state_reasons, AttrType::keyword};
    SingleValue<std::string> printer_static_resource_directory_uri{
        AttrName::printer_static_resource_directory_uri, AttrType::uri};
    SingleValue<int> printer_static_resource_k_octets_free{
        AttrName::printer_static_resource_k_octets_free, AttrType::integer};
    SingleValue<int> printer_static_resource_k_octets_supported{
        AttrName::printer_static_resource_k_octets_supported,
        AttrType::integer};
    SetOfValues<std::string> printer_strings_languages_supported{
        AttrName::printer_strings_languages_supported,
        AttrType::naturalLanguage};
    SingleValue<std::string> printer_strings_uri{AttrName::printer_strings_uri,
                                                 AttrType::uri};
    SetOfValues<std::string> printer_supply{AttrName::printer_supply,
                                            AttrType::octetString};
    SetOfValues<StringWithLanguage> printer_supply_description{
        AttrName::printer_supply_description, AttrType::text};
    SingleValue<std::string> printer_supply_info_uri{
        AttrName::printer_supply_info_uri, AttrType::uri};
    SingleValue<int> printer_up_time{AttrName::printer_up_time,
                                     AttrType::integer};
    SetOfValues<std::string> printer_uri_supported{
        AttrName::printer_uri_supported, AttrType::uri};
    SingleValue<std::string> printer_uuid{AttrName::printer_uuid,
                                          AttrType::uri};
    SetOfCollections<C_printer_xri_supported> printer_xri_supported{
        AttrName::printer_xri_supported};
    SingleCollection<C_proof_print_default> proof_print_default{
        AttrName::proof_print_default};
    SetOfValues<E_proof_print_supported> proof_print_supported{
        AttrName::proof_print_supported, AttrType::keyword};
    SingleValue<int> punching_hole_diameter_configured{
        AttrName::punching_hole_diameter_configured, AttrType::integer};
    SetOfValues<RangeOfInteger> punching_locations_supported{
        AttrName::punching_locations_supported, AttrType::rangeOfInteger};
    SetOfValues<RangeOfInteger> punching_offset_supported{
        AttrName::punching_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_punching_reference_edge_supported>
        punching_reference_edge_supported{
            AttrName::punching_reference_edge_supported, AttrType::keyword};
    SetOfValues<Resolution> pwg_raster_document_resolution_supported{
        AttrName::pwg_raster_document_resolution_supported,
        AttrType::resolution};
    SingleValue<E_pwg_raster_document_sheet_back>
        pwg_raster_document_sheet_back{AttrName::pwg_raster_document_sheet_back,
                                       AttrType::keyword};
    SetOfValues<E_pwg_raster_document_type_supported>
        pwg_raster_document_type_supported{
            AttrName::pwg_raster_document_type_supported, AttrType::keyword};
    SingleValue<int> queued_job_count{AttrName::queued_job_count,
                                      AttrType::integer};
    SetOfValues<std::string> reference_uri_schemes_supported{
        AttrName::reference_uri_schemes_supported, AttrType::uriScheme};
    SingleValue<bool> requesting_user_uri_supported{
        AttrName::requesting_user_uri_supported, AttrType::boolean};
    SetOfValues<E_save_disposition_supported> save_disposition_supported{
        AttrName::save_disposition_supported, AttrType::keyword};
    SingleValue<std::string> save_document_format_default{
        AttrName::save_document_format_default, AttrType::mimeMediaType};
    SetOfValues<std::string> save_document_format_supported{
        AttrName::save_document_format_supported, AttrType::mimeMediaType};
    SingleValue<std::string> save_location_default{
        AttrName::save_location_default, AttrType::uri};
    SetOfValues<std::string> save_location_supported{
        AttrName::save_location_supported, AttrType::uri};
    SingleValue<bool> save_name_subdirectory_supported{
        AttrName::save_name_subdirectory_supported, AttrType::boolean};
    SingleValue<bool> save_name_supported{AttrName::save_name_supported,
                                          AttrType::boolean};
    SingleCollection<C_separator_sheets_default> separator_sheets_default{
        AttrName::separator_sheets_default};
    SingleValue<E_sheet_collate_default> sheet_collate_default{
        AttrName::sheet_collate_default, AttrType::keyword};
    SetOfValues<E_sheet_collate_supported> sheet_collate_supported{
        AttrName::sheet_collate_supported, AttrType::keyword};
    SingleValue<E_sides_default> sides_default{AttrName::sides_default,
                                               AttrType::keyword};
    SetOfValues<E_sides_supported> sides_supported{AttrName::sides_supported,
                                                   AttrType::keyword};
    SetOfValues<RangeOfInteger> stitching_angle_supported{
        AttrName::stitching_angle_supported, AttrType::rangeOfInteger};
    SetOfValues<RangeOfInteger> stitching_locations_supported{
        AttrName::stitching_locations_supported, AttrType::rangeOfInteger};
    SetOfValues<E_stitching_method_supported> stitching_method_supported{
        AttrName::stitching_method_supported, AttrType::keyword};
    SetOfValues<RangeOfInteger> stitching_offset_supported{
        AttrName::stitching_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_stitching_reference_edge_supported>
        stitching_reference_edge_supported{
            AttrName::stitching_reference_edge_supported, AttrType::keyword};
    SetOfValues<std::string> subordinate_printers_supported{
        AttrName::subordinate_printers_supported, AttrType::uri};
    SetOfValues<RangeOfInteger> trimming_offset_supported{
        AttrName::trimming_offset_supported, AttrType::rangeOfInteger};
    SetOfValues<E_trimming_reference_edge_supported>
        trimming_reference_edge_supported{
            AttrName::trimming_reference_edge_supported, AttrType::keyword};
    SetOfValues<E_trimming_type_supported> trimming_type_supported{
        AttrName::trimming_type_supported, AttrType::keyword};
    SetOfValues<E_trimming_when_supported> trimming_when_supported{
        AttrName::trimming_when_supported, AttrType::keyword};
    SetOfValues<E_uri_authentication_supported> uri_authentication_supported{
        AttrName::uri_authentication_supported, AttrType::keyword};
    SetOfValues<E_uri_security_supported> uri_security_supported{
        AttrName::uri_security_supported, AttrType::keyword};
    SetOfValues<E_which_jobs_supported> which_jobs_supported{
        AttrName::which_jobs_supported, AttrType::keyword};
    SingleValue<E_x_image_position_default> x_image_position_default{
        AttrName::x_image_position_default, AttrType::keyword};
    SetOfValues<E_x_image_position_supported> x_image_position_supported{
        AttrName::x_image_position_supported, AttrType::keyword};
    SingleValue<int> x_image_shift_default{AttrName::x_image_shift_default,
                                           AttrType::integer};
    SingleValue<RangeOfInteger> x_image_shift_supported{
        AttrName::x_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> x_side1_image_shift_default{
        AttrName::x_side1_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> x_side1_image_shift_supported{
        AttrName::x_side1_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> x_side2_image_shift_default{
        AttrName::x_side2_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> x_side2_image_shift_supported{
        AttrName::x_side2_image_shift_supported, AttrType::rangeOfInteger};
    SetOfValues<E_xri_authentication_supported> xri_authentication_supported{
        AttrName::xri_authentication_supported, AttrType::keyword};
    SetOfValues<E_xri_security_supported> xri_security_supported{
        AttrName::xri_security_supported, AttrType::keyword};
    SetOfValues<std::string> xri_uri_scheme_supported{
        AttrName::xri_uri_scheme_supported, AttrType::uriScheme};
    SingleValue<E_y_image_position_default> y_image_position_default{
        AttrName::y_image_position_default, AttrType::keyword};
    SetOfValues<E_y_image_position_supported> y_image_position_supported{
        AttrName::y_image_position_supported, AttrType::keyword};
    SingleValue<int> y_image_shift_default{AttrName::y_image_shift_default,
                                           AttrType::integer};
    SingleValue<RangeOfInteger> y_image_shift_supported{
        AttrName::y_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> y_side1_image_shift_default{
        AttrName::y_side1_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> y_side1_image_shift_supported{
        AttrName::y_side1_image_shift_supported, AttrType::rangeOfInteger};
    SingleValue<int> y_side2_image_shift_default{
        AttrName::y_side2_image_shift_default, AttrType::integer};
    SingleValue<RangeOfInteger> y_side2_image_shift_supported{
        AttrName::y_side2_image_shift_supported, AttrType::rangeOfInteger};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_printer_attributes> printer_attributes{
      GroupTag::printer_attributes};
  Response_Get_Printer_Attributes();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Hold_Job : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> message{AttrName::message, AttrType::text};
    SingleValue<E_job_hold_until> job_hold_until{AttrName::job_hold_until,
                                                 AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Hold_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Hold_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Hold_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Pause_Printer : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Pause_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Pause_Printer : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Pause_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Print_Job : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> job_name{AttrName::job_name,
                                             AttrType::name};
    SingleValue<bool> ipp_attribute_fidelity{AttrName::ipp_attribute_fidelity,
                                             AttrType::boolean};
    SingleValue<StringWithLanguage> document_name{AttrName::document_name,
                                                  AttrType::name};
    SingleValue<E_compression> compression{AttrName::compression,
                                           AttrType::keyword};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    SingleValue<std::string> document_natural_language{
        AttrName::document_natural_language, AttrType::naturalLanguage};
    SingleValue<int> job_k_octets{AttrName::job_k_octets, AttrType::integer};
    SingleValue<int> job_impressions{AttrName::job_impressions,
                                     AttrType::integer};
    SingleValue<int> job_media_sheets{AttrName::job_media_sheets,
                                      AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Request_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_Print_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Print_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  typedef Response_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Print_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Print_URI : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<std::string> document_uri{AttrName::document_uri,
                                          AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> job_name{AttrName::job_name,
                                             AttrType::name};
    SingleValue<bool> ipp_attribute_fidelity{AttrName::ipp_attribute_fidelity,
                                             AttrType::boolean};
    SingleValue<StringWithLanguage> document_name{AttrName::document_name,
                                                  AttrType::name};
    SingleValue<E_compression> compression{AttrName::compression,
                                           AttrType::keyword};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    SingleValue<std::string> document_natural_language{
        AttrName::document_natural_language, AttrType::naturalLanguage};
    SingleValue<int> job_k_octets{AttrName::job_k_octets, AttrType::integer};
    SingleValue<int> job_impressions{AttrName::job_impressions,
                                     AttrType::integer};
    SingleValue<int> job_media_sheets{AttrName::job_media_sheets,
                                      AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Request_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_Print_URI();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Print_URI : public Response {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<StringWithLanguage> status_message{AttrName::status_message,
                                                   AttrType::text};
    SingleValue<StringWithLanguage> detailed_status_message{
        AttrName::detailed_status_message, AttrType::text};
    SingleValue<StringWithLanguage> document_access_error{
        AttrName::document_access_error, AttrType::text};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  typedef Response_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Print_URI();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Release_Job : public Request {
  typedef Request_Cancel_Job::G_operation_attributes G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Release_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Release_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Release_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Resume_Printer : public Request {
  typedef Request_Pause_Printer::G_operation_attributes G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Resume_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Resume_Printer : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Resume_Printer();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Send_Document : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> document_name{AttrName::document_name,
                                                  AttrType::name};
    SingleValue<E_compression> compression{AttrName::compression,
                                           AttrType::keyword};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    SingleValue<std::string> document_natural_language{
        AttrName::document_natural_language, AttrType::naturalLanguage};
    SingleValue<bool> last_document{AttrName::last_document, AttrType::boolean};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Send_Document();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Send_Document : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  typedef Response_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Send_Document();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Send_URI : public Request {
  struct G_operation_attributes : public Collection {
    SingleValue<std::string> attributes_charset{AttrName::attributes_charset,
                                                AttrType::charset};
    SingleValue<std::string> attributes_natural_language{
        AttrName::attributes_natural_language, AttrType::naturalLanguage};
    SingleValue<std::string> printer_uri{AttrName::printer_uri, AttrType::uri};
    SingleValue<int> job_id{AttrName::job_id, AttrType::integer};
    SingleValue<std::string> job_uri{AttrName::job_uri, AttrType::uri};
    SingleValue<StringWithLanguage> requesting_user_name{
        AttrName::requesting_user_name, AttrType::name};
    SingleValue<StringWithLanguage> document_name{AttrName::document_name,
                                                  AttrType::name};
    SingleValue<E_compression> compression{AttrName::compression,
                                           AttrType::keyword};
    SingleValue<std::string> document_format{AttrName::document_format,
                                             AttrType::mimeMediaType};
    SingleValue<std::string> document_natural_language{
        AttrName::document_natural_language, AttrType::naturalLanguage};
    SingleValue<bool> last_document{AttrName::last_document, AttrType::boolean};
    SingleValue<std::string> document_uri{AttrName::document_uri,
                                          AttrType::uri};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  Request_Send_URI();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Send_URI : public Response {
  typedef Response_Print_URI::G_operation_attributes G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  typedef Response_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Response_Send_URI();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Request_Validate_Job : public Request {
  typedef Request_Print_Job::G_operation_attributes G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Request_Create_Job::G_job_attributes G_job_attributes;
  SingleGroup<G_job_attributes> job_attributes{GroupTag::job_attributes};
  Request_Validate_Job();
  std::vector<Group*> GetKnownGroups() override;
};
struct IPP_EXPORT Response_Validate_Job : public Response {
  typedef Response_CUPS_Add_Modify_Class::G_operation_attributes
      G_operation_attributes;
  SingleGroup<G_operation_attributes> operation_attributes{
      GroupTag::operation_attributes};
  typedef Response_CUPS_Authenticate_Job::G_unsupported_attributes
      G_unsupported_attributes;
  SingleGroup<G_unsupported_attributes> unsupported_attributes{
      GroupTag::unsupported_attributes};
  Response_Validate_Job();
  std::vector<Group*> GetKnownGroups() override;
};
}  // namespace ipp

#endif  //  LIBIPP_IPP_OPERATIONS_H_
