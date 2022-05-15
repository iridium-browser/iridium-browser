// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_COLLECTIONS_H_
#define LIBIPP_IPP_COLLECTIONS_H_

#include <string>
#include <vector>

#include "ipp_attribute.h"  // NOLINT(build/include)
#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_export.h"  // NOLINT(build/include)
#include "ipp_package.h"  // NOLINT(build/include)

// This file contains definition of classes corresponding to supported IPP
// collection attributes. See ipp.h for more details.
// This file was generated from IPP schema based on the following sources:
// * [rfc8011]
// * [CUPS Implementation of IPP] at https://www.cups.org/doc/spec-ipp.html
// * [IPP registry] at https://www.pwg.org/ipp/ipp-registrations.xml

namespace ipp {
struct IPP_EXPORT C_job_constraints_supported : public Collection {
  SingleValue<StringWithLanguage> resolver_name{AttrName::resolver_name,
                                                AttrType::name};
  std::vector<Attribute*> GetKnownAttributes() override;
};
struct IPP_EXPORT C_job_finishings_col_actual : public Collection {
  struct C_media_size : public Collection {
    SingleValue<int> x_dimension{AttrName::x_dimension, AttrType::integer};
    SingleValue<int> y_dimension{AttrName::y_dimension, AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleValue<E_media_back_coating> media_back_coating{
      AttrName::media_back_coating, AttrType::keyword};
  SingleValue<int> media_bottom_margin{AttrName::media_bottom_margin,
                                       AttrType::integer};
  SingleValue<E_media_color> media_color{AttrName::media_color,
                                         AttrType::keyword};
  SingleValue<E_media_front_coating> media_front_coating{
      AttrName::media_front_coating, AttrType::keyword};
  SingleValue<E_media_grain> media_grain{AttrName::media_grain,
                                         AttrType::keyword};
  SingleValue<int> media_hole_count{AttrName::media_hole_count,
                                    AttrType::integer};
  SingleValue<StringWithLanguage> media_info{AttrName::media_info,
                                             AttrType::text};
  SingleValue<E_media_key> media_key{AttrName::media_key, AttrType::keyword};
  SingleValue<int> media_left_margin{AttrName::media_left_margin,
                                     AttrType::integer};
  SingleValue<int> media_order_count{AttrName::media_order_count,
                                     AttrType::integer};
  SingleValue<E_media_pre_printed> media_pre_printed{
      AttrName::media_pre_printed, AttrType::keyword};
  SingleValue<E_media_recycled> media_recycled{AttrName::media_recycled,
                                               AttrType::keyword};
  SingleValue<int> media_right_margin{AttrName::media_right_margin,
                                      AttrType::integer};
  SingleCollection<C_media_size> media_size{AttrName::media_size};
  SingleValue<StringWithLanguage> media_size_name{AttrName::media_size_name,
                                                  AttrType::name};
  SingleValue<E_media_source> media_source{AttrName::media_source,
                                           AttrType::keyword};
  SingleValue<int> media_thickness{AttrName::media_thickness,
                                   AttrType::integer};
  SingleValue<E_media_tooth> media_tooth{AttrName::media_tooth,
                                         AttrType::keyword};
  SingleValue<int> media_top_margin{AttrName::media_top_margin,
                                    AttrType::integer};
  SingleValue<E_media_type> media_type{AttrName::media_type, AttrType::keyword};
  SingleValue<int> media_weight_metric{AttrName::media_weight_metric,
                                       AttrType::integer};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_constraints_supported C_job_resolvers_supported;
struct IPP_EXPORT C_job_save_disposition : public Collection {
  struct C_save_info : public Collection {
    SingleValue<std::string> save_document_format{
        AttrName::save_document_format, AttrType::mimeMediaType};
    SingleValue<std::string> save_location{AttrName::save_location,
                                           AttrType::uri};
    SingleValue<StringWithLanguage> save_name{AttrName::save_name,
                                              AttrType::name};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleValue<E_save_disposition> save_disposition{AttrName::save_disposition,
                                                   AttrType::keyword};
  SetOfCollections<C_save_info> save_info{AttrName::save_info};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_finishings_col_actual C_media_col;
typedef C_job_finishings_col_actual C_media_col_actual;
struct IPP_EXPORT C_media_col_database : public Collection {
  struct C_media_size : public Collection {
    SingleValue<int> x_dimension{AttrName::x_dimension, AttrType::integer};
    SingleValue<int> y_dimension{AttrName::y_dimension, AttrType::integer};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_media_source_properties : public Collection {
    SingleValue<E_media_source_feed_direction> media_source_feed_direction{
        AttrName::media_source_feed_direction, AttrType::keyword};
    SingleValue<E_media_source_feed_orientation> media_source_feed_orientation{
        AttrName::media_source_feed_orientation, AttrType::enum_};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleValue<E_media_back_coating> media_back_coating{
      AttrName::media_back_coating, AttrType::keyword};
  SingleValue<int> media_bottom_margin{AttrName::media_bottom_margin,
                                       AttrType::integer};
  SingleValue<E_media_color> media_color{AttrName::media_color,
                                         AttrType::keyword};
  SingleValue<E_media_front_coating> media_front_coating{
      AttrName::media_front_coating, AttrType::keyword};
  SingleValue<E_media_grain> media_grain{AttrName::media_grain,
                                         AttrType::keyword};
  SingleValue<int> media_hole_count{AttrName::media_hole_count,
                                    AttrType::integer};
  SingleValue<StringWithLanguage> media_info{AttrName::media_info,
                                             AttrType::text};
  SingleValue<E_media_key> media_key{AttrName::media_key, AttrType::keyword};
  SingleValue<int> media_left_margin{AttrName::media_left_margin,
                                     AttrType::integer};
  SingleValue<int> media_order_count{AttrName::media_order_count,
                                     AttrType::integer};
  SingleValue<E_media_pre_printed> media_pre_printed{
      AttrName::media_pre_printed, AttrType::keyword};
  SingleValue<E_media_recycled> media_recycled{AttrName::media_recycled,
                                               AttrType::keyword};
  SingleValue<int> media_right_margin{AttrName::media_right_margin,
                                      AttrType::integer};
  SingleCollection<C_media_size> media_size{AttrName::media_size};
  SingleValue<StringWithLanguage> media_size_name{AttrName::media_size_name,
                                                  AttrType::name};
  SingleValue<E_media_source> media_source{AttrName::media_source,
                                           AttrType::keyword};
  SingleCollection<C_media_source_properties> media_source_properties{
      AttrName::media_source_properties};
  SingleValue<int> media_thickness{AttrName::media_thickness,
                                   AttrType::integer};
  SingleValue<E_media_tooth> media_tooth{AttrName::media_tooth,
                                         AttrType::keyword};
  SingleValue<int> media_top_margin{AttrName::media_top_margin,
                                    AttrType::integer};
  SingleValue<E_media_type> media_type{AttrName::media_type, AttrType::keyword};
  SingleValue<int> media_weight_metric{AttrName::media_weight_metric,
                                       AttrType::integer};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_finishings_col_actual C_media_col_default;
typedef C_media_col_database C_media_col_ready;
struct IPP_EXPORT C_media_size_supported : public Collection {
  SingleValue<RangeOfInteger> x_dimension{AttrName::x_dimension,
                                          AttrType::rangeOfInteger};
  SingleValue<RangeOfInteger> y_dimension{AttrName::y_dimension,
                                          AttrType::rangeOfInteger};
  std::vector<Attribute*> GetKnownAttributes() override;
};
struct IPP_EXPORT C_pdl_init_file : public Collection {
  SingleValue<StringWithLanguage> pdl_init_file_entry{
      AttrName::pdl_init_file_entry, AttrType::name};
  SingleValue<std::string> pdl_init_file_location{
      AttrName::pdl_init_file_location, AttrType::uri};
  SingleValue<StringWithLanguage> pdl_init_file_name{
      AttrName::pdl_init_file_name, AttrType::name};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_pdl_init_file C_pdl_init_file_default;
struct IPP_EXPORT C_printer_contact_col : public Collection {
  SingleValue<StringWithLanguage> contact_name{AttrName::contact_name,
                                               AttrType::name};
  SingleValue<std::string> contact_uri{AttrName::contact_uri, AttrType::uri};
  SetOfValues<StringWithLanguage> contact_vcard{AttrName::contact_vcard,
                                                AttrType::text};
  std::vector<Attribute*> GetKnownAttributes() override;
};
struct IPP_EXPORT C_printer_icc_profiles : public Collection {
  SingleValue<StringWithLanguage> profile_name{AttrName::profile_name,
                                               AttrType::name};
  SingleValue<std::string> profile_url{AttrName::profile_url, AttrType::uri};
  std::vector<Attribute*> GetKnownAttributes() override;
};
struct IPP_EXPORT C_printer_xri_supported : public Collection {
  SingleValue<E_xri_authentication> xri_authentication{
      AttrName::xri_authentication, AttrType::keyword};
  SingleValue<E_xri_security> xri_security{AttrName::xri_security,
                                           AttrType::keyword};
  SingleValue<std::string> xri_uri{AttrName::xri_uri, AttrType::uri};
  std::vector<Attribute*> GetKnownAttributes() override;
};
struct IPP_EXPORT C_proof_print : public Collection {
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  SingleValue<int> proof_print_copies{AttrName::proof_print_copies,
                                      AttrType::integer};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_proof_print C_proof_print_default;
struct IPP_EXPORT C_separator_sheets : public Collection {
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  SetOfValues<E_separator_sheets_type> separator_sheets_type{
      AttrName::separator_sheets_type, AttrType::keyword};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_separator_sheets C_separator_sheets_actual;
typedef C_separator_sheets C_separator_sheets_default;
struct IPP_EXPORT C_cover_back : public Collection {
  SingleValue<E_cover_type> cover_type{AttrName::cover_type, AttrType::keyword};
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_cover_back C_cover_back_actual;
typedef C_cover_back C_cover_back_default;
typedef C_cover_back C_cover_front;
typedef C_cover_back C_cover_front_actual;
typedef C_cover_back C_cover_front_default;
struct IPP_EXPORT C_document_format_details_default : public Collection {
  SingleValue<std::string> document_format{AttrName::document_format,
                                           AttrType::mimeMediaType};
  SingleValue<StringWithLanguage> document_format_device_id{
      AttrName::document_format_device_id, AttrType::text};
  SingleValue<StringWithLanguage> document_format_version{
      AttrName::document_format_version, AttrType::text};
  SetOfValues<std::string> document_natural_language{
      AttrName::document_natural_language, AttrType::naturalLanguage};
  SingleValue<StringWithLanguage> document_source_application_name{
      AttrName::document_source_application_name, AttrType::name};
  SingleValue<StringWithLanguage> document_source_application_version{
      AttrName::document_source_application_version, AttrType::text};
  SingleValue<StringWithLanguage> document_source_os_name{
      AttrName::document_source_os_name, AttrType::name};
  SingleValue<StringWithLanguage> document_source_os_version{
      AttrName::document_source_os_version, AttrType::text};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_document_format_details_default C_document_format_details_supplied;
struct IPP_EXPORT C_finishings_col : public Collection {
  struct C_baling : public Collection {
    SingleValue<E_baling_type> baling_type{AttrName::baling_type,
                                           AttrType::keyword};
    SingleValue<E_baling_when> baling_when{AttrName::baling_when,
                                           AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_binding : public Collection {
    SingleValue<E_binding_reference_edge> binding_reference_edge{
        AttrName::binding_reference_edge, AttrType::keyword};
    SingleValue<E_binding_type> binding_type{AttrName::binding_type,
                                             AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_coating : public Collection {
    SingleValue<E_coating_sides> coating_sides{AttrName::coating_sides,
                                               AttrType::keyword};
    SingleValue<E_coating_type> coating_type{AttrName::coating_type,
                                             AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_covering : public Collection {
    SingleValue<E_covering_name> covering_name{AttrName::covering_name,
                                               AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_folding : public Collection {
    SingleValue<E_folding_direction> folding_direction{
        AttrName::folding_direction, AttrType::keyword};
    SingleValue<int> folding_offset{AttrName::folding_offset,
                                    AttrType::integer};
    SingleValue<E_folding_reference_edge> folding_reference_edge{
        AttrName::folding_reference_edge, AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_laminating : public Collection {
    SingleValue<E_laminating_sides> laminating_sides{AttrName::laminating_sides,
                                                     AttrType::keyword};
    SingleValue<E_laminating_type> laminating_type{AttrName::laminating_type,
                                                   AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_media_size : public Collection {
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_punching : public Collection {
    SetOfValues<int> punching_locations{AttrName::punching_locations,
                                        AttrType::integer};
    SingleValue<int> punching_offset{AttrName::punching_offset,
                                     AttrType::integer};
    SingleValue<E_punching_reference_edge> punching_reference_edge{
        AttrName::punching_reference_edge, AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_stitching : public Collection {
    SingleValue<int> stitching_angle{AttrName::stitching_angle,
                                     AttrType::integer};
    SetOfValues<int> stitching_locations{AttrName::stitching_locations,
                                         AttrType::integer};
    SingleValue<E_stitching_method> stitching_method{AttrName::stitching_method,
                                                     AttrType::keyword};
    SingleValue<int> stitching_offset{AttrName::stitching_offset,
                                      AttrType::integer};
    SingleValue<E_stitching_reference_edge> stitching_reference_edge{
        AttrName::stitching_reference_edge, AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  struct C_trimming : public Collection {
    SingleValue<int> trimming_offset{AttrName::trimming_offset,
                                     AttrType::integer};
    SingleValue<E_trimming_reference_edge> trimming_reference_edge{
        AttrName::trimming_reference_edge, AttrType::keyword};
    SingleValue<E_trimming_type> trimming_type{AttrName::trimming_type,
                                               AttrType::keyword};
    SingleValue<E_trimming_when> trimming_when{AttrName::trimming_when,
                                               AttrType::keyword};
    std::vector<Attribute*> GetKnownAttributes() override;
  };
  SingleCollection<C_baling> baling{AttrName::baling};
  SingleCollection<C_binding> binding{AttrName::binding};
  SingleCollection<C_coating> coating{AttrName::coating};
  SingleCollection<C_covering> covering{AttrName::covering};
  SingleValue<E_finishing_template> finishing_template{
      AttrName::finishing_template, AttrType::keyword};
  SetOfCollections<C_folding> folding{AttrName::folding};
  SingleValue<E_imposition_template> imposition_template{
      AttrName::imposition_template, AttrType::keyword};
  SingleCollection<C_laminating> laminating{AttrName::laminating};
  SingleValue<RangeOfInteger> media_sheets_supported{
      AttrName::media_sheets_supported, AttrType::rangeOfInteger};
  SingleCollection<C_media_size> media_size{AttrName::media_size};
  SingleValue<StringWithLanguage> media_size_name{AttrName::media_size_name,
                                                  AttrType::name};
  SingleCollection<C_punching> punching{AttrName::punching};
  SingleCollection<C_stitching> stitching{AttrName::stitching};
  SetOfCollections<C_trimming> trimming{AttrName::trimming};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_finishings_col C_finishings_col_actual;
typedef C_finishings_col C_finishings_col_database;
typedef C_finishings_col C_finishings_col_default;
typedef C_finishings_col C_finishings_col_ready;
struct IPP_EXPORT C_insert_sheet : public Collection {
  SingleValue<int> insert_after_page_number{AttrName::insert_after_page_number,
                                            AttrType::integer};
  SingleValue<int> insert_count{AttrName::insert_count, AttrType::integer};
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_insert_sheet C_insert_sheet_actual;
typedef C_insert_sheet C_insert_sheet_default;
struct IPP_EXPORT C_job_accounting_sheets : public Collection {
  SingleValue<E_job_accounting_output_bin> job_accounting_output_bin{
      AttrName::job_accounting_output_bin, AttrType::keyword};
  SingleValue<E_job_accounting_sheets_type> job_accounting_sheets_type{
      AttrName::job_accounting_sheets_type, AttrType::keyword};
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_accounting_sheets C_job_accounting_sheets_actual;
typedef C_job_accounting_sheets C_job_accounting_sheets_default;
typedef C_cover_back C_job_cover_back;
typedef C_cover_back C_job_cover_back_actual;
typedef C_cover_back C_job_cover_back_default;
typedef C_cover_back C_job_cover_front;
typedef C_cover_back C_job_cover_front_actual;
typedef C_cover_back C_job_cover_front_default;
struct IPP_EXPORT C_job_error_sheet : public Collection {
  SingleValue<E_job_error_sheet_type> job_error_sheet_type{
      AttrName::job_error_sheet_type, AttrType::keyword};
  SingleValue<E_job_error_sheet_when> job_error_sheet_when{
      AttrName::job_error_sheet_when, AttrType::keyword};
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_error_sheet C_job_error_sheet_actual;
typedef C_job_error_sheet C_job_error_sheet_default;
typedef C_finishings_col C_job_finishings_col;
typedef C_finishings_col C_job_finishings_col_default;
typedef C_finishings_col C_job_finishings_col_ready;
struct IPP_EXPORT C_job_sheets_col : public Collection {
  SingleValue<E_job_sheets> job_sheets{AttrName::job_sheets, AttrType::keyword};
  SingleValue<E_media> media{AttrName::media, AttrType::keyword};
  SingleCollection<C_media_col> media_col{AttrName::media_col};
  std::vector<Attribute*> GetKnownAttributes() override;
};
typedef C_job_sheets_col C_job_sheets_col_actual;
typedef C_job_sheets_col C_job_sheets_col_default;
struct IPP_EXPORT C_overrides : public Collection {
  SingleValue<int> copies{AttrName::copies, AttrType::integer};
  SingleCollection<C_cover_back> cover_back{AttrName::cover_back};
  SingleCollection<C_cover_front> cover_front{AttrName::cover_front};
  SetOfValues<RangeOfInteger> document_copies{AttrName::document_copies,
                                              AttrType::rangeOfInteger};
  SetOfValues<RangeOfInteger> document_numbers{AttrName::document_numbers,
                                               AttrType::rangeOfInteger};
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
  SingleValue<StringWithLanguage> job_sheet_message{AttrName::job_sheet_message,
                                                    AttrType::text};
  SingleValue<E_job_sheets> job_sheets{AttrName::job_sheets, AttrType::keyword};
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
  SingleValue<E_output_bin> output_bin{AttrName::output_bin, AttrType::keyword};
  SingleValue<StringWithLanguage> output_device{AttrName::output_device,
                                                AttrType::name};
  SingleValue<E_page_delivery> page_delivery{AttrName::page_delivery,
                                             AttrType::keyword};
  SingleValue<E_page_order_received> page_order_received{
      AttrName::page_order_received, AttrType::keyword};
  SetOfValues<RangeOfInteger> page_ranges{AttrName::page_ranges,
                                          AttrType::rangeOfInteger};
  SetOfValues<RangeOfInteger> pages{AttrName::pages, AttrType::rangeOfInteger};
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
typedef C_overrides C_overrides_actual;
}  // namespace ipp

#endif  //  LIBIPP_IPP_COLLECTIONS_H_
