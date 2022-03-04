// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_collections.h"

namespace ipp {
std::vector<Attribute*> C_cover_back::GetKnownAttributes() {
  return {&cover_type, &media, &media_col};
}
std::vector<Attribute*>
C_document_format_details_default::GetKnownAttributes() {
  return {&document_format,
          &document_format_device_id,
          &document_format_version,
          &document_natural_language,
          &document_source_application_name,
          &document_source_application_version,
          &document_source_os_name,
          &document_source_os_version};
}
std::vector<Attribute*> C_finishings_col::GetKnownAttributes() {
  return {&baling,
          &binding,
          &coating,
          &covering,
          &finishing_template,
          &folding,
          &imposition_template,
          &laminating,
          &media_sheets_supported,
          &media_size,
          &media_size_name,
          &punching,
          &stitching,
          &trimming};
}
std::vector<Attribute*> C_finishings_col::C_baling::GetKnownAttributes() {
  return {&baling_type, &baling_when};
}
std::vector<Attribute*> C_finishings_col::C_binding::GetKnownAttributes() {
  return {&binding_reference_edge, &binding_type};
}
std::vector<Attribute*> C_finishings_col::C_coating::GetKnownAttributes() {
  return {&coating_sides, &coating_type};
}
std::vector<Attribute*> C_finishings_col::C_covering::GetKnownAttributes() {
  return {&covering_name};
}
std::vector<Attribute*> C_finishings_col::C_folding::GetKnownAttributes() {
  return {&folding_direction, &folding_offset, &folding_reference_edge};
}
std::vector<Attribute*> C_finishings_col::C_laminating::GetKnownAttributes() {
  return {&laminating_sides, &laminating_type};
}
std::vector<Attribute*> C_finishings_col::C_media_size::GetKnownAttributes() {
  return {};
}
std::vector<Attribute*> C_finishings_col::C_punching::GetKnownAttributes() {
  return {&punching_locations, &punching_offset, &punching_reference_edge};
}
std::vector<Attribute*> C_finishings_col::C_stitching::GetKnownAttributes() {
  return {&stitching_angle, &stitching_locations, &stitching_method,
          &stitching_offset, &stitching_reference_edge};
}
std::vector<Attribute*> C_finishings_col::C_trimming::GetKnownAttributes() {
  return {&trimming_offset, &trimming_reference_edge, &trimming_type,
          &trimming_when};
}
std::vector<Attribute*> C_insert_sheet::GetKnownAttributes() {
  return {&insert_after_page_number, &insert_count, &media, &media_col};
}
std::vector<Attribute*> C_job_accounting_sheets::GetKnownAttributes() {
  return {&job_accounting_output_bin, &job_accounting_sheets_type, &media,
          &media_col};
}
std::vector<Attribute*> C_job_constraints_supported::GetKnownAttributes() {
  return {&resolver_name};
}
std::vector<Attribute*> C_job_error_sheet::GetKnownAttributes() {
  return {&job_error_sheet_type, &job_error_sheet_when, &media, &media_col};
}
std::vector<Attribute*> C_job_finishings_col_actual::GetKnownAttributes() {
  return {&media_back_coating,  &media_bottom_margin, &media_color,
          &media_front_coating, &media_grain,         &media_hole_count,
          &media_info,          &media_key,           &media_left_margin,
          &media_order_count,   &media_pre_printed,   &media_recycled,
          &media_right_margin,  &media_size,          &media_size_name,
          &media_source,        &media_thickness,     &media_tooth,
          &media_top_margin,    &media_type,          &media_weight_metric};
}
std::vector<Attribute*>
C_job_finishings_col_actual::C_media_size::GetKnownAttributes() {
  return {&x_dimension, &y_dimension};
}
std::vector<Attribute*> C_job_save_disposition::GetKnownAttributes() {
  return {&save_disposition, &save_info};
}
std::vector<Attribute*>
C_job_save_disposition::C_save_info::GetKnownAttributes() {
  return {&save_document_format, &save_location, &save_name};
}
std::vector<Attribute*> C_job_sheets_col::GetKnownAttributes() {
  return {&job_sheets, &media, &media_col};
}
std::vector<Attribute*> C_media_col_database::GetKnownAttributes() {
  return {&media_back_coating,
          &media_bottom_margin,
          &media_color,
          &media_front_coating,
          &media_grain,
          &media_hole_count,
          &media_info,
          &media_key,
          &media_left_margin,
          &media_order_count,
          &media_pre_printed,
          &media_recycled,
          &media_right_margin,
          &media_size,
          &media_size_name,
          &media_source,
          &media_source_properties,
          &media_thickness,
          &media_tooth,
          &media_top_margin,
          &media_type,
          &media_weight_metric};
}
std::vector<Attribute*>
C_media_col_database::C_media_size::GetKnownAttributes() {
  return {&x_dimension, &y_dimension};
}
std::vector<Attribute*>
C_media_col_database::C_media_source_properties::GetKnownAttributes() {
  return {&media_source_feed_direction, &media_source_feed_orientation};
}
std::vector<Attribute*> C_media_size_supported::GetKnownAttributes() {
  return {&x_dimension, &y_dimension};
}
std::vector<Attribute*> C_overrides::GetKnownAttributes() {
  return {&copies,
          &cover_back,
          &cover_front,
          &document_copies,
          &document_numbers,
          &feed_orientation,
          &finishings,
          &finishings_col,
          &font_name_requested,
          &font_size_requested,
          &force_front_side,
          &imposition_template,
          &insert_sheet,
          &job_account_id,
          &job_account_type,
          &job_accounting_sheets,
          &job_accounting_user_id,
          &job_copies,
          &job_cover_back,
          &job_cover_front,
          &job_delay_output_until,
          &job_delay_output_until_time,
          &job_error_action,
          &job_error_sheet,
          &job_finishings,
          &job_finishings_col,
          &job_hold_until,
          &job_hold_until_time,
          &job_message_to_operator,
          &job_pages_per_set,
          &job_phone_number,
          &job_priority,
          &job_recipient_name,
          &job_save_disposition,
          &job_sheet_message,
          &job_sheets,
          &job_sheets_col,
          &media,
          &media_col,
          &media_input_tray_check,
          &multiple_document_handling,
          &number_up,
          &orientation_requested,
          &output_bin,
          &output_device,
          &page_delivery,
          &page_order_received,
          &page_ranges,
          &pages,
          &pages_per_subset,
          &pdl_init_file,
          &presentation_direction_number_up,
          &print_color_mode,
          &print_content_optimize,
          &print_quality,
          &print_rendering_intent,
          &print_scaling,
          &printer_resolution,
          &proof_print,
          &separator_sheets,
          &sheet_collate,
          &sides,
          &x_image_position,
          &x_image_shift,
          &x_side1_image_shift,
          &x_side2_image_shift,
          &y_image_position,
          &y_image_shift,
          &y_side1_image_shift,
          &y_side2_image_shift};
}
std::vector<Attribute*> C_pdl_init_file::GetKnownAttributes() {
  return {&pdl_init_file_entry, &pdl_init_file_location, &pdl_init_file_name};
}
std::vector<Attribute*> C_printer_contact_col::GetKnownAttributes() {
  return {&contact_name, &contact_uri, &contact_vcard};
}
std::vector<Attribute*> C_printer_icc_profiles::GetKnownAttributes() {
  return {&profile_name, &profile_url};
}
std::vector<Attribute*> C_printer_xri_supported::GetKnownAttributes() {
  return {&xri_authentication, &xri_security, &xri_uri};
}
std::vector<Attribute*> C_proof_print::GetKnownAttributes() {
  return {&media, &media_col, &proof_print_copies};
}
std::vector<Attribute*> C_separator_sheets::GetKnownAttributes() {
  return {&media, &media_col, &separator_sheets_type};
}
}  // namespace ipp
