// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_enums.h"

#include <gtest/gtest.h>

namespace ipp {

// Tests conversion between single enum's element and its string value.
// If string_value == "", the value has no string representation.
template <typename TEnum>
void TestEnumValue(TEnum enum_value, const std::string& string_value) {
  EXPECT_EQ(ToString(enum_value), string_value);
  TEnum out_value = static_cast<TEnum>(0xabcd);
  if (string_value.empty()) {
    EXPECT_FALSE(FromString(string_value, &out_value));
    // no changes to output parameter
    EXPECT_EQ(out_value, static_cast<TEnum>(0xabcd));
  } else {
    EXPECT_TRUE(FromString(string_value, &out_value));
    EXPECT_EQ(out_value, enum_value);
  }
  // FromString() must not crash for nullptr
  TEnum* out_value_ptr = nullptr;
  EXPECT_FALSE(FromString(string_value, out_value_ptr));
}

// Tests the same as TestEnumValue plus two general functions:
// ToString(attr_name,value) and FromString(string,attr_name,&out_value).
template <typename TEnum>
void TestKeywordValue(AttrName attr_name,
                      TEnum enum_value,
                      const std::string& string_value) {
  TestEnumValue(enum_value, string_value);
  EXPECT_EQ(ToString(attr_name, static_cast<int>(enum_value)), string_value);
  int out_value = 123456789;
  if (string_value.empty()) {
    EXPECT_FALSE(FromString(string_value, attr_name, &out_value));
    // no changes to output parameter
    EXPECT_EQ(out_value, 123456789);
  } else {
    EXPECT_TRUE(FromString(string_value, attr_name, &out_value));
    EXPECT_EQ(out_value, static_cast<int>(enum_value));
  }
}

TEST(enums, GroupTag) {
  TestEnumValue(GroupTag::document_attributes, "document-attributes");
  TestEnumValue(GroupTag::unsupported_attributes, "unsupported-attributes");
  TestEnumValue(GroupTag::operation_attributes, "operation-attributes");
  TestEnumValue(GroupTag::system_attributes, "system-attributes");
  // value 3 corresponds to end-of-attributes-tag [rfc8010], it cannot be here
  TestEnumValue(static_cast<GroupTag>(3), "");
}

TEST(enums, AttrName) {
  TestEnumValue(AttrName::_unknown, "");
  TestEnumValue(AttrName::attributes_charset, "attributes-charset");
  TestEnumValue(AttrName::y_side2_image_shift_supported,
                "y-side2-image-shift-supported");
}

TEST(enums, KeywordsAndEnumerations) {
  TestKeywordValue(AttrName::auth_info_required, E_auth_info_required::domain,
                   "domain");
  TestKeywordValue(AttrName::auth_info_required, E_auth_info_required::username,
                   "username");
  TestKeywordValue(AttrName::current_page_order,
                   E_current_page_order::_1_to_n_order, "1-to-n-order");
  TestKeywordValue(AttrName::current_page_order,
                   E_current_page_order::n_to_1_order, "n-to-1-order");
  TestKeywordValue(AttrName::y_image_position_supported,
                   E_y_image_position_supported::bottom, "bottom");
  TestKeywordValue(AttrName::y_image_position_supported,
                   E_y_image_position_supported::top, "top");
}

TEST(enums, ValuesFor0) {
  TestEnumValue(E_auth_info_required::domain, "domain");
  TestEnumValue(E_baling_type::band, "band");
  TestEnumValue(E_baling_when::after_job, "after-job");
  TestEnumValue(E_binding_reference_edge::bottom, "bottom");
  TestEnumValue(E_binding_type::adhesive, "adhesive");
  TestEnumValue(E_coating_sides::back, "back");
  TestEnumValue(E_coating_type::archival, "archival");
  TestEnumValue(E_compression::compress, "compress");
  TestEnumValue(E_cover_back_supported::cover_type, "cover-type");
  TestEnumValue(E_cover_type::no_cover, "no-cover");
  TestEnumValue(E_covering_name::plain, "plain");
  TestEnumValue(E_current_page_order::_1_to_n_order, "1-to-n-order");
  TestEnumValue(E_document_digital_signature::dss, "dss");
  TestEnumValue(E_document_format_details_supported::document_format,
                "document-format");
  TestEnumValue(E_document_format_varying_attributes::none, "none");
  TestEnumValue(E_feed_orientation::long_edge_first, "long-edge-first");
  TestEnumValue(E_finishing_template::bale, "bale");
  TestEnumValue(E_folding_direction::inward, "inward");
  TestEnumValue(E_identify_actions::display, "display");
  TestEnumValue(E_imposition_template::none, "none");
  TestEnumValue(E_input_sides::one_sided, "one-sided");
  TestEnumValue(E_ipp_features_supported::document_object, "document-object");
  TestEnumValue(E_job_account_type::general, "general");
  TestEnumValue(E_job_accounting_output_bin::auto_, "auto");
  TestEnumValue(E_job_accounting_sheets_type::none, "none");
  TestEnumValue(E_job_delay_output_until::day_time, "day-time");
  TestEnumValue(E_job_error_action::abort_job, "abort-job");
  TestEnumValue(E_job_error_sheet_when::always, "always");
  TestEnumValue(E_job_hold_until::day_time, "day-time");
  TestEnumValue(E_job_password_encryption::md2, "md2");
  TestEnumValue(E_job_sheets::first_print_stream_page,
                "first-print-stream-page");
  TestEnumValue(E_job_spooling_supported::automatic, "automatic");
  TestEnumValue(E_job_state_reasons::aborted_by_system, "aborted-by-system");
  TestEnumValue(E_laminating_type::archival, "archival");
  TestEnumValue(E_material_color::black, "black");
  TestEnumValue(E_media::a, "a");
  TestEnumValue(E_media_back_coating::glossy, "glossy");
  TestEnumValue(E_media_grain::x_direction, "x-direction");
  TestEnumValue(E_media_input_tray_check::bottom, "bottom");
  TestEnumValue(E_media_pre_printed::blank, "blank");
  TestEnumValue(E_media_ready::a, "a");
  TestEnumValue(E_media_source::alternate, "alternate");
  TestEnumValue(E_media_tooth::antique, "antique");
  TestEnumValue(E_media_type::aluminum, "aluminum");
  TestEnumValue(
      E_multiple_document_handling::separate_documents_collated_copies,
      "separate-documents-collated-copies");
  TestEnumValue(E_multiple_operation_time_out_action::abort_job, "abort-job");
  TestEnumValue(E_notify_events::document_completed, "document-completed");
  TestEnumValue(E_notify_pull_method::ippget, "ippget");
  TestEnumValue(E_page_delivery::reverse_order_face_down,
                "reverse-order-face-down");
  TestEnumValue(E_pdf_versions_supported::adobe_1_3, "adobe-1.3");
  TestEnumValue(E_pdl_init_file_supported::pdl_init_file_entry,
                "pdl-init-file-entry");
  TestEnumValue(E_pdl_override_supported::attempted, "attempted");
  TestEnumValue(E_presentation_direction_number_up::tobottom_toleft,
                "tobottom-toleft");
  TestEnumValue(E_print_color_mode::auto_, "auto");
  TestEnumValue(E_print_content_optimize::auto_, "auto");
  TestEnumValue(E_print_rendering_intent::absolute, "absolute");
  TestEnumValue(E_print_scaling::auto_, "auto");
  TestEnumValue(E_printer_state_reasons::alert_removal_of_binary_change_entry,
                "alert-removal-of-binary-change-entry");
  TestEnumValue(E_proof_print_supported::media, "media");
  TestEnumValue(E_pwg_raster_document_sheet_back::flipped, "flipped");
  TestEnumValue(E_pwg_raster_document_type_supported::adobe_rgb_8,
                "adobe-rgb_8");
  TestEnumValue(E_requested_attributes::all, "all");
  TestEnumValue(E_save_disposition::none, "none");
  TestEnumValue(E_separator_sheets_type::both_sheets, "both-sheets");
  TestEnumValue(E_sheet_collate::collated, "collated");
  TestEnumValue(E_status_code::successful_ok, "successful-ok");
  TestEnumValue(E_stitching_method::auto_, "auto");
  TestEnumValue(E_stitching_reference_edge::bottom, "bottom");
  TestEnumValue(E_trimming_type::draw_line, "draw-line");
  TestEnumValue(E_trimming_when::after_documents, "after-documents");
  TestEnumValue(E_uri_authentication_supported::basic, "basic");
  TestEnumValue(E_uri_security_supported::none, "none");
  TestEnumValue(E_which_jobs::aborted, "aborted");
  TestEnumValue(E_x_image_position::center, "center");
  TestEnumValue(E_y_image_position::bottom, "bottom");
}

TEST(enums, FirstValuesForEnumsWithout0) {
  TestEnumValue(E_finishings::none, "none");
  TestEnumValue(E_input_orientation_requested::portrait, "portrait");
  TestEnumValue(E_input_quality::draft, "draft");
  TestEnumValue(E_ipp_versions_supported::_1_0, "1.0");
  TestEnumValue(E_job_collation_type::uncollated_sheets, "uncollated-sheets");
  TestEnumValue(E_job_state::pending, "pending");
  TestEnumValue(E_operations_supported::Print_Job, "Print-Job");
  TestEnumValue(E_printer_state::idle, "idle");
}

TEST(enums, ValuesFor222) {
  TestEnumValue(static_cast<GroupTag>(222), "");
  TestEnumValue(static_cast<AttrName>(222), "job-pages");
  TestEnumValue(static_cast<E_auth_info_required>(222), "");
  TestEnumValue(static_cast<E_baling_type>(222), "");
  TestEnumValue(static_cast<E_baling_when>(222), "");
  TestEnumValue(static_cast<E_binding_reference_edge>(222), "");
  TestEnumValue(static_cast<E_binding_type>(222), "");
  TestEnumValue(static_cast<E_coating_sides>(222), "");
  TestEnumValue(static_cast<E_coating_type>(222), "");
  TestEnumValue(static_cast<E_compression>(222), "");
  TestEnumValue(static_cast<E_cover_back_supported>(222), "");
  TestEnumValue(static_cast<E_cover_type>(222), "");
  TestEnumValue(static_cast<E_covering_name>(222), "");
  TestEnumValue(static_cast<E_current_page_order>(222), "");
  TestEnumValue(static_cast<E_document_digital_signature>(222), "");
  TestEnumValue(static_cast<E_document_format_details_supported>(222), "");
  TestEnumValue(static_cast<E_document_format_varying_attributes>(222), "");
  TestEnumValue(static_cast<E_feed_orientation>(222), "");
  TestEnumValue(static_cast<E_finishing_template>(222), "");
  TestEnumValue(static_cast<E_finishings>(222), "");
  TestEnumValue(static_cast<E_folding_direction>(222), "");
  TestEnumValue(static_cast<E_identify_actions>(222), "");
  TestEnumValue(static_cast<E_imposition_template>(222), "");
  TestEnumValue(static_cast<E_input_orientation_requested>(222), "");
  TestEnumValue(static_cast<E_input_quality>(222), "");
  TestEnumValue(static_cast<E_input_sides>(222), "");
  TestEnumValue(static_cast<E_ipp_features_supported>(222), "");
  TestEnumValue(static_cast<E_ipp_versions_supported>(222), "");
  TestEnumValue(static_cast<E_job_account_type>(222), "");
  TestEnumValue(static_cast<E_job_accounting_output_bin>(222), "");
  TestEnumValue(static_cast<E_job_accounting_sheets_type>(222), "");
  TestEnumValue(static_cast<E_job_collation_type>(222), "");
  TestEnumValue(static_cast<E_job_delay_output_until>(222), "");
  TestEnumValue(static_cast<E_job_error_action>(222), "");
  TestEnumValue(static_cast<E_job_error_sheet_when>(222), "");
  TestEnumValue(static_cast<E_job_hold_until>(222), "");
  TestEnumValue(static_cast<E_job_mandatory_attributes>(222), "");
  TestEnumValue(static_cast<E_job_password_encryption>(222), "");
  TestEnumValue(static_cast<E_job_sheets>(222), "");
  TestEnumValue(static_cast<E_job_spooling_supported>(222), "");
  TestEnumValue(static_cast<E_job_state>(222), "");
  TestEnumValue(static_cast<E_job_state_reasons>(222), "");
  TestEnumValue(static_cast<E_laminating_type>(222), "");
  TestEnumValue(static_cast<E_material_color>(222), "");
  TestEnumValue(static_cast<E_media>(222), "iso-b5-white");
  TestEnumValue(static_cast<E_media_back_coating>(222), "");
  TestEnumValue(static_cast<E_media_grain>(222), "");
  TestEnumValue(static_cast<E_media_input_tray_check>(222), "");
  TestEnumValue(static_cast<E_media_key>(222), "");
  TestEnumValue(static_cast<E_media_pre_printed>(222), "");
  TestEnumValue(static_cast<E_media_ready>(222), "iso-b9");
  TestEnumValue(static_cast<E_media_source>(222), "");
  TestEnumValue(static_cast<E_media_tooth>(222), "");
  TestEnumValue(static_cast<E_media_type>(222), "");
  TestEnumValue(static_cast<E_multiple_document_handling>(222), "");
  TestEnumValue(static_cast<E_multiple_operation_time_out_action>(222), "");
  TestEnumValue(static_cast<E_notify_events>(222), "");
  TestEnumValue(static_cast<E_notify_pull_method>(222), "");
  TestEnumValue(static_cast<E_operations_supported>(222), "");
  TestEnumValue(static_cast<E_page_delivery>(222), "");
  TestEnumValue(static_cast<E_pdf_versions_supported>(222), "");
  TestEnumValue(static_cast<E_pdl_init_file_supported>(222), "");
  TestEnumValue(static_cast<E_pdl_override_supported>(222), "");
  TestEnumValue(static_cast<E_presentation_direction_number_up>(222), "");
  TestEnumValue(static_cast<E_print_color_mode>(222), "");
  TestEnumValue(static_cast<E_print_content_optimize>(222), "");
  TestEnumValue(static_cast<E_print_rendering_intent>(222), "");
  TestEnumValue(static_cast<E_print_scaling>(222), "");
  TestEnumValue(static_cast<E_printer_state>(222), "");
  TestEnumValue(static_cast<E_printer_state_reasons>(222), "inserter-closed");
  TestEnumValue(static_cast<E_proof_print_supported>(222), "");
  TestEnumValue(static_cast<E_pwg_raster_document_sheet_back>(222), "");
  TestEnumValue(static_cast<E_pwg_raster_document_type_supported>(222), "");
  TestEnumValue(static_cast<E_requested_attributes>(222), "");
  TestEnumValue(static_cast<E_save_disposition>(222), "");
  TestEnumValue(static_cast<E_separator_sheets_type>(222), "");
  TestEnumValue(static_cast<E_sheet_collate>(222), "");
  TestEnumValue(static_cast<E_status_code>(222), "");
  TestEnumValue(static_cast<E_stitching_method>(222), "");
  TestEnumValue(static_cast<E_stitching_reference_edge>(222), "");
  TestEnumValue(static_cast<E_trimming_type>(222), "");
  TestEnumValue(static_cast<E_trimming_when>(222), "");
  TestEnumValue(static_cast<E_uri_authentication_supported>(222), "");
  TestEnumValue(static_cast<E_uri_security_supported>(222), "");
  TestEnumValue(static_cast<E_which_jobs>(222), "");
  TestEnumValue(static_cast<E_x_image_position>(222), "");
  TestEnumValue(static_cast<E_xri_authentication>(222), "");
  TestEnumValue(static_cast<E_xri_security>(222), "");
  TestEnumValue(static_cast<E_y_image_position>(222), "");
}

}  // namespace ipp
