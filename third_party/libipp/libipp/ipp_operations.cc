// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_operations.h"

#include <memory>

namespace ipp {
std::unique_ptr<Request> Request::NewRequest(Operation id) {
  switch (id) {
    case Operation::CUPS_Add_Modify_Class:
      return std::make_unique<Request_CUPS_Add_Modify_Class>();
    case Operation::CUPS_Add_Modify_Printer:
      return std::make_unique<Request_CUPS_Add_Modify_Printer>();
    case Operation::CUPS_Authenticate_Job:
      return std::make_unique<Request_CUPS_Authenticate_Job>();
    case Operation::CUPS_Create_Local_Printer:
      return std::make_unique<Request_CUPS_Create_Local_Printer>();
    case Operation::CUPS_Delete_Class:
      return std::make_unique<Request_CUPS_Delete_Class>();
    case Operation::CUPS_Delete_Printer:
      return std::make_unique<Request_CUPS_Delete_Printer>();
    case Operation::CUPS_Get_Classes:
      return std::make_unique<Request_CUPS_Get_Classes>();
    case Operation::CUPS_Get_Default:
      return std::make_unique<Request_CUPS_Get_Default>();
    case Operation::CUPS_Get_Document:
      return std::make_unique<Request_CUPS_Get_Document>();
    case Operation::CUPS_Get_Printers:
      return std::make_unique<Request_CUPS_Get_Printers>();
    case Operation::CUPS_Move_Job:
      return std::make_unique<Request_CUPS_Move_Job>();
    case Operation::CUPS_Set_Default:
      return std::make_unique<Request_CUPS_Set_Default>();
    case Operation::Cancel_Job:
      return std::make_unique<Request_Cancel_Job>();
    case Operation::Create_Job:
      return std::make_unique<Request_Create_Job>();
    case Operation::Get_Job_Attributes:
      return std::make_unique<Request_Get_Job_Attributes>();
    case Operation::Get_Jobs:
      return std::make_unique<Request_Get_Jobs>();
    case Operation::Get_Printer_Attributes:
      return std::make_unique<Request_Get_Printer_Attributes>();
    case Operation::Hold_Job:
      return std::make_unique<Request_Hold_Job>();
    case Operation::Pause_Printer:
      return std::make_unique<Request_Pause_Printer>();
    case Operation::Print_Job:
      return std::make_unique<Request_Print_Job>();
    case Operation::Print_URI:
      return std::make_unique<Request_Print_URI>();
    case Operation::Release_Job:
      return std::make_unique<Request_Release_Job>();
    case Operation::Resume_Printer:
      return std::make_unique<Request_Resume_Printer>();
    case Operation::Send_Document:
      return std::make_unique<Request_Send_Document>();
    case Operation::Send_URI:
      return std::make_unique<Request_Send_URI>();
    case Operation::Validate_Job:
      return std::make_unique<Request_Validate_Job>();
    default:
      return nullptr;
  }
  return nullptr;
}
std::unique_ptr<Response> Response::NewResponse(Operation id) {
  switch (id) {
    case Operation::CUPS_Add_Modify_Class:
      return std::make_unique<Response_CUPS_Add_Modify_Class>();
    case Operation::CUPS_Add_Modify_Printer:
      return std::make_unique<Response_CUPS_Add_Modify_Printer>();
    case Operation::CUPS_Authenticate_Job:
      return std::make_unique<Response_CUPS_Authenticate_Job>();
    case Operation::CUPS_Create_Local_Printer:
      return std::make_unique<Response_CUPS_Create_Local_Printer>();
    case Operation::CUPS_Delete_Class:
      return std::make_unique<Response_CUPS_Delete_Class>();
    case Operation::CUPS_Delete_Printer:
      return std::make_unique<Response_CUPS_Delete_Printer>();
    case Operation::CUPS_Get_Classes:
      return std::make_unique<Response_CUPS_Get_Classes>();
    case Operation::CUPS_Get_Default:
      return std::make_unique<Response_CUPS_Get_Default>();
    case Operation::CUPS_Get_Document:
      return std::make_unique<Response_CUPS_Get_Document>();
    case Operation::CUPS_Get_Printers:
      return std::make_unique<Response_CUPS_Get_Printers>();
    case Operation::CUPS_Move_Job:
      return std::make_unique<Response_CUPS_Move_Job>();
    case Operation::CUPS_Set_Default:
      return std::make_unique<Response_CUPS_Set_Default>();
    case Operation::Cancel_Job:
      return std::make_unique<Response_Cancel_Job>();
    case Operation::Create_Job:
      return std::make_unique<Response_Create_Job>();
    case Operation::Get_Job_Attributes:
      return std::make_unique<Response_Get_Job_Attributes>();
    case Operation::Get_Jobs:
      return std::make_unique<Response_Get_Jobs>();
    case Operation::Get_Printer_Attributes:
      return std::make_unique<Response_Get_Printer_Attributes>();
    case Operation::Hold_Job:
      return std::make_unique<Response_Hold_Job>();
    case Operation::Pause_Printer:
      return std::make_unique<Response_Pause_Printer>();
    case Operation::Print_Job:
      return std::make_unique<Response_Print_Job>();
    case Operation::Print_URI:
      return std::make_unique<Response_Print_URI>();
    case Operation::Release_Job:
      return std::make_unique<Response_Release_Job>();
    case Operation::Resume_Printer:
      return std::make_unique<Response_Resume_Printer>();
    case Operation::Send_Document:
      return std::make_unique<Response_Send_Document>();
    case Operation::Send_URI:
      return std::make_unique<Response_Send_URI>();
    case Operation::Validate_Job:
      return std::make_unique<Response_Validate_Job>();
    default:
      return nullptr;
  }
  return nullptr;
}
Request_CUPS_Add_Modify_Class::Request_CUPS_Add_Modify_Class()
    : Request(Operation::CUPS_Add_Modify_Class) {}
std::vector<Group*> Request_CUPS_Add_Modify_Class::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*>
Request_CUPS_Add_Modify_Class::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language, &printer_uri};
}
std::vector<Attribute*>
Request_CUPS_Add_Modify_Class::G_printer_attributes::GetKnownAttributes() {
  return {&auth_info_required,
          &member_uris,
          &printer_is_accepting_jobs,
          &printer_info,
          &printer_location,
          &printer_more_info,
          &printer_state,
          &printer_state_message,
          &requesting_user_name_allowed,
          &requesting_user_name_denied};
}
Response_CUPS_Add_Modify_Class::Response_CUPS_Add_Modify_Class()
    : Response(Operation::CUPS_Add_Modify_Class) {}
std::vector<Group*> Response_CUPS_Add_Modify_Class::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Response_CUPS_Add_Modify_Class::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language, &status_message,
          &detailed_status_message};
}
Request_CUPS_Add_Modify_Printer::Request_CUPS_Add_Modify_Printer()
    : Request(Operation::CUPS_Add_Modify_Printer) {}
std::vector<Group*> Request_CUPS_Add_Modify_Printer::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*>
Request_CUPS_Add_Modify_Printer::G_printer_attributes::GetKnownAttributes() {
  return {&auth_info_required,
          &job_sheets_default,
          &device_uri,
          &printer_is_accepting_jobs,
          &printer_info,
          &printer_location,
          &printer_more_info,
          &printer_state,
          &printer_state_message,
          &requesting_user_name_allowed,
          &requesting_user_name_denied};
}
Response_CUPS_Add_Modify_Printer::Response_CUPS_Add_Modify_Printer()
    : Response(Operation::CUPS_Add_Modify_Printer) {}
std::vector<Group*> Response_CUPS_Add_Modify_Printer::GetKnownGroups() {
  return {&operation_attributes};
}
Request_CUPS_Authenticate_Job::Request_CUPS_Authenticate_Job()
    : Request(Operation::CUPS_Authenticate_Job) {}
std::vector<Group*> Request_CUPS_Authenticate_Job::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
std::vector<Attribute*>
Request_CUPS_Authenticate_Job::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language, &printer_uri,
          &job_id, &job_uri};
}
std::vector<Attribute*>
Request_CUPS_Authenticate_Job::G_job_attributes::GetKnownAttributes() {
  return {&auth_info, &job_hold_until};
}
Response_CUPS_Authenticate_Job::Response_CUPS_Authenticate_Job()
    : Response(Operation::CUPS_Authenticate_Job) {}
std::vector<Group*> Response_CUPS_Authenticate_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
std::vector<Attribute*>
Response_CUPS_Authenticate_Job::G_unsupported_attributes::GetKnownAttributes() {
  return {};
}
Request_CUPS_Create_Local_Printer::Request_CUPS_Create_Local_Printer()
    : Request(Operation::CUPS_Create_Local_Printer) {}
std::vector<Group*> Request_CUPS_Create_Local_Printer::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*> Request_CUPS_Create_Local_Printer::
    G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language};
}
std::vector<Attribute*>
Request_CUPS_Create_Local_Printer::G_printer_attributes::GetKnownAttributes() {
  return {&printer_name,         &device_uri,   &printer_device_id,
          &printer_geo_location, &printer_info, &printer_location};
}
Response_CUPS_Create_Local_Printer::Response_CUPS_Create_Local_Printer()
    : Response(Operation::CUPS_Create_Local_Printer) {}
std::vector<Group*> Response_CUPS_Create_Local_Printer::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*>
Response_CUPS_Create_Local_Printer::G_printer_attributes::GetKnownAttributes() {
  return {&printer_id, &printer_is_accepting_jobs, &printer_state,
          &printer_state_reasons, &printer_uri_supported};
}
Request_CUPS_Delete_Class::Request_CUPS_Delete_Class()
    : Request(Operation::CUPS_Delete_Class) {}
std::vector<Group*> Request_CUPS_Delete_Class::GetKnownGroups() {
  return {&operation_attributes};
}
Response_CUPS_Delete_Class::Response_CUPS_Delete_Class()
    : Response(Operation::CUPS_Delete_Class) {}
std::vector<Group*> Response_CUPS_Delete_Class::GetKnownGroups() {
  return {&operation_attributes};
}
Request_CUPS_Delete_Printer::Request_CUPS_Delete_Printer()
    : Request(Operation::CUPS_Delete_Printer) {}
std::vector<Group*> Request_CUPS_Delete_Printer::GetKnownGroups() {
  return {&operation_attributes};
}
Response_CUPS_Delete_Printer::Response_CUPS_Delete_Printer()
    : Response(Operation::CUPS_Delete_Printer) {}
std::vector<Group*> Response_CUPS_Delete_Printer::GetKnownGroups() {
  return {&operation_attributes};
}
Request_CUPS_Get_Classes::Request_CUPS_Get_Classes()
    : Request(Operation::CUPS_Get_Classes) {}
std::vector<Group*> Request_CUPS_Get_Classes::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_CUPS_Get_Classes::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language,
          &first_printer_name, &limit,
          &printer_location,   &printer_type,
          &printer_type_mask,  &requested_attributes,
          &requested_user_name};
}
Response_CUPS_Get_Classes::Response_CUPS_Get_Classes()
    : Response(Operation::CUPS_Get_Classes) {}
std::vector<Group*> Response_CUPS_Get_Classes::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*>
Response_CUPS_Get_Classes::G_printer_attributes::GetKnownAttributes() {
  return {&member_names, &member_uris};
}
Request_CUPS_Get_Default::Request_CUPS_Get_Default()
    : Request(Operation::CUPS_Get_Default) {}
std::vector<Group*> Request_CUPS_Get_Default::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_CUPS_Get_Default::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language,
          &requested_attributes};
}
Response_CUPS_Get_Default::Response_CUPS_Get_Default()
    : Response(Operation::CUPS_Get_Default) {}
std::vector<Group*> Response_CUPS_Get_Default::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
std::vector<Attribute*>
Response_CUPS_Get_Default::G_printer_attributes::GetKnownAttributes() {
  return {&baling_type_supported,
          &baling_when_supported,
          &binding_reference_edge_supported,
          &binding_type_supported,
          &charset_configured,
          &charset_supported,
          &coating_sides_supported,
          &coating_type_supported,
          &color_supported,
          &compression_supported,
          &copies_default,
          &copies_supported,
          &cover_back_default,
          &cover_back_supported,
          &cover_front_default,
          &cover_front_supported,
          &covering_name_supported,
          &document_charset_default,
          &document_charset_supported,
          &document_digital_signature_default,
          &document_digital_signature_supported,
          &document_format_default,
          &document_format_details_default,
          &document_format_details_supported,
          &document_format_supported,
          &document_format_version_default,
          &document_format_version_supported,
          &document_natural_language_default,
          &document_natural_language_supported,
          &document_password_supported,
          &feed_orientation_supported,
          &finishing_template_supported,
          &finishings_col_database,
          &finishings_col_default,
          &finishings_col_ready,
          &finishings_default,
          &finishings_ready,
          &finishings_supported,
          &folding_direction_supported,
          &folding_offset_supported,
          &folding_reference_edge_supported,
          &font_name_requested_default,
          &font_name_requested_supported,
          &font_size_requested_default,
          &font_size_requested_supported,
          &generated_natural_language_supported,
          &identify_actions_default,
          &identify_actions_supported,
          &insert_after_page_number_supported,
          &insert_count_supported,
          &insert_sheet_default,
          &ipp_features_supported,
          &ipp_versions_supported,
          &ippget_event_life,
          &job_account_id_default,
          &job_account_id_supported,
          &job_account_type_default,
          &job_account_type_supported,
          &job_accounting_sheets_default,
          &job_accounting_user_id_default,
          &job_accounting_user_id_supported,
          &job_authorization_uri_supported,
          &job_constraints_supported,
          &job_copies_default,
          &job_copies_supported,
          &job_cover_back_default,
          &job_cover_back_supported,
          &job_cover_front_default,
          &job_cover_front_supported,
          &job_delay_output_until_default,
          &job_delay_output_until_supported,
          &job_delay_output_until_time_supported,
          &job_error_action_default,
          &job_error_action_supported,
          &job_error_sheet_default,
          &job_finishings_col_default,
          &job_finishings_col_ready,
          &job_finishings_default,
          &job_finishings_ready,
          &job_finishings_supported,
          &job_hold_until_default,
          &job_hold_until_supported,
          &job_hold_until_time_supported,
          &job_ids_supported,
          &job_impressions_supported,
          &job_k_octets_supported,
          &job_media_sheets_supported,
          &job_message_to_operator_default,
          &job_message_to_operator_supported,
          &job_pages_per_set_supported,
          &job_password_encryption_supported,
          &job_password_supported,
          &job_phone_number_default,
          &job_phone_number_supported,
          &job_priority_default,
          &job_priority_supported,
          &job_recipient_name_default,
          &job_recipient_name_supported,
          &job_resolvers_supported,
          &job_sheet_message_default,
          &job_sheet_message_supported,
          &job_sheets_col_default,
          &job_sheets_default,
          &job_sheets_supported,
          &job_spooling_supported,
          &jpeg_k_octets_supported,
          &jpeg_x_dimension_supported,
          &jpeg_y_dimension_supported,
          &laminating_sides_supported,
          &laminating_type_supported,
          &max_save_info_supported,
          &max_stitching_locations_supported,
          &media_back_coating_supported,
          &media_bottom_margin_supported,
          &media_col_database,
          &media_col_default,
          &media_col_ready,
          &media_color_supported,
          &media_default,
          &media_front_coating_supported,
          &media_grain_supported,
          &media_hole_count_supported,
          &media_info_supported,
          &media_left_margin_supported,
          &media_order_count_supported,
          &media_pre_printed_supported,
          &media_ready,
          &media_recycled_supported,
          &media_right_margin_supported,
          &media_size_supported,
          &media_source_supported,
          &media_supported,
          &media_thickness_supported,
          &media_tooth_supported,
          &media_top_margin_supported,
          &media_type_supported,
          &media_weight_metric_supported,
          &multiple_document_handling_default,
          &multiple_document_handling_supported,
          &multiple_document_jobs_supported,
          &multiple_operation_time_out,
          &multiple_operation_time_out_action,
          &natural_language_configured,
          &notify_events_default,
          &notify_events_supported,
          &notify_lease_duration_default,
          &notify_lease_duration_supported,
          &notify_pull_method_supported,
          &notify_schemes_supported,
          &number_up_default,
          &number_up_supported,
          &oauth_authorization_server_uri,
          &operations_supported,
          &orientation_requested_default,
          &orientation_requested_supported,
          &output_bin_default,
          &output_bin_supported,
          &output_device_supported,
          &output_device_uuid_supported,
          &page_delivery_default,
          &page_delivery_supported,
          &page_order_received_default,
          &page_order_received_supported,
          &page_ranges_supported,
          &pages_per_subset_supported,
          &parent_printers_supported,
          &pdf_k_octets_supported,
          &pdf_versions_supported,
          &pdl_init_file_default,
          &pdl_init_file_entry_supported,
          &pdl_init_file_location_supported,
          &pdl_init_file_name_subdirectory_supported,
          &pdl_init_file_name_supported,
          &pdl_init_file_supported,
          &pdl_override_supported,
          &preferred_attributes_supported,
          &presentation_direction_number_up_default,
          &presentation_direction_number_up_supported,
          &print_color_mode_default,
          &print_color_mode_supported,
          &print_content_optimize_default,
          &print_content_optimize_supported,
          &print_quality_default,
          &print_quality_supported,
          &print_rendering_intent_default,
          &print_rendering_intent_supported,
          &printer_charge_info,
          &printer_charge_info_uri,
          &printer_contact_col,
          &printer_current_time,
          &printer_device_id,
          &printer_dns_sd_name,
          &printer_driver_installer,
          &printer_geo_location,
          &printer_icc_profiles,
          &printer_icons,
          &printer_info,
          &printer_location,
          &printer_make_and_model,
          &printer_more_info_manufacturer,
          &printer_name,
          &printer_organization,
          &printer_organizational_unit,
          &printer_resolution_default,
          &printer_resolution_supported,
          &printer_static_resource_directory_uri,
          &printer_static_resource_k_octets_supported,
          &printer_strings_languages_supported,
          &printer_strings_uri,
          &printer_xri_supported,
          &proof_print_default,
          &proof_print_supported,
          &punching_hole_diameter_configured,
          &punching_locations_supported,
          &punching_offset_supported,
          &punching_reference_edge_supported,
          &pwg_raster_document_resolution_supported,
          &pwg_raster_document_sheet_back,
          &pwg_raster_document_type_supported,
          &reference_uri_schemes_supported,
          &requesting_user_uri_supported,
          &save_disposition_supported,
          &save_document_format_default,
          &save_document_format_supported,
          &save_location_default,
          &save_location_supported,
          &save_name_subdirectory_supported,
          &save_name_supported,
          &separator_sheets_default,
          &sheet_collate_default,
          &sheet_collate_supported,
          &sides_default,
          &sides_supported,
          &stitching_angle_supported,
          &stitching_locations_supported,
          &stitching_method_supported,
          &stitching_offset_supported,
          &stitching_reference_edge_supported,
          &subordinate_printers_supported,
          &trimming_offset_supported,
          &trimming_reference_edge_supported,
          &trimming_type_supported,
          &trimming_when_supported,
          &uri_authentication_supported,
          &uri_security_supported,
          &which_jobs_supported,
          &x_image_position_default,
          &x_image_position_supported,
          &x_image_shift_default,
          &x_image_shift_supported,
          &x_side1_image_shift_default,
          &x_side1_image_shift_supported,
          &x_side2_image_shift_default,
          &x_side2_image_shift_supported,
          &y_image_position_default,
          &y_image_position_supported,
          &y_image_shift_default,
          &y_image_shift_supported,
          &y_side1_image_shift_default,
          &y_side1_image_shift_supported,
          &y_side2_image_shift_default,
          &y_side2_image_shift_supported};
}
Request_CUPS_Get_Document::Request_CUPS_Get_Document()
    : Request(Operation::CUPS_Get_Document) {}
std::vector<Group*> Request_CUPS_Get_Document::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_CUPS_Get_Document::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &document_number};
}
Response_CUPS_Get_Document::Response_CUPS_Get_Document()
    : Response(Operation::CUPS_Get_Document) {}
std::vector<Group*> Response_CUPS_Get_Document::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Response_CUPS_Get_Document::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language,
          &status_message,     &detailed_status_message,
          &document_format,    &document_number,
          &document_name};
}
Request_CUPS_Get_Printers::Request_CUPS_Get_Printers()
    : Request(Operation::CUPS_Get_Printers) {}
std::vector<Group*> Request_CUPS_Get_Printers::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_CUPS_Get_Printers::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,   &attributes_natural_language,
          &first_printer_name,   &limit,
          &printer_id,           &printer_location,
          &printer_type,         &printer_type_mask,
          &requested_attributes, &requested_user_name};
}
Response_CUPS_Get_Printers::Response_CUPS_Get_Printers()
    : Response(Operation::CUPS_Get_Printers) {}
std::vector<Group*> Response_CUPS_Get_Printers::GetKnownGroups() {
  return {&operation_attributes, &printer_attributes};
}
Request_CUPS_Move_Job::Request_CUPS_Move_Job()
    : Request(Operation::CUPS_Move_Job) {}
std::vector<Group*> Request_CUPS_Move_Job::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
std::vector<Attribute*>
Request_CUPS_Move_Job::G_job_attributes::GetKnownAttributes() {
  return {&job_printer_uri};
}
Response_CUPS_Move_Job::Response_CUPS_Move_Job()
    : Response(Operation::CUPS_Move_Job) {}
std::vector<Group*> Response_CUPS_Move_Job::GetKnownGroups() {
  return {&operation_attributes};
}
Request_CUPS_Set_Default::Request_CUPS_Set_Default()
    : Request(Operation::CUPS_Set_Default) {}
std::vector<Group*> Request_CUPS_Set_Default::GetKnownGroups() {
  return {&operation_attributes};
}
Response_CUPS_Set_Default::Response_CUPS_Set_Default()
    : Response(Operation::CUPS_Set_Default) {}
std::vector<Group*> Response_CUPS_Set_Default::GetKnownGroups() {
  return {&operation_attributes};
}
Request_Cancel_Job::Request_Cancel_Job() : Request(Operation::Cancel_Job) {}
std::vector<Group*> Request_Cancel_Job::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Cancel_Job::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &requesting_user_name,
          &message};
}
Response_Cancel_Job::Response_Cancel_Job() : Response(Operation::Cancel_Job) {}
std::vector<Group*> Response_Cancel_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
Request_Create_Job::Request_Create_Job() : Request(Operation::Create_Job) {}
std::vector<Group*> Request_Create_Job::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
std::vector<Attribute*>
Request_Create_Job::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language,
          &printer_uri,        &requesting_user_name,
          &job_name,           &ipp_attribute_fidelity,
          &job_k_octets,       &job_impressions,
          &job_media_sheets};
}
std::vector<Attribute*>
Request_Create_Job::G_job_attributes::GetKnownAttributes() {
  return {&copies,
          &cover_back,
          &cover_front,
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
          &overrides,
          &page_delivery,
          &page_order_received,
          &page_ranges,
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
Response_Create_Job::Response_Create_Job() : Response(Operation::Create_Job) {}
std::vector<Group*> Response_Create_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
std::vector<Attribute*>
Response_Create_Job::G_job_attributes::GetKnownAttributes() {
  return {&job_id,
          &job_uri,
          &job_state,
          &job_state_reasons,
          &job_state_message,
          &number_of_intervening_jobs};
}
Request_Get_Job_Attributes::Request_Get_Job_Attributes()
    : Request(Operation::Get_Job_Attributes) {}
std::vector<Group*> Request_Get_Job_Attributes::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Get_Job_Attributes::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &requesting_user_name,
          &requested_attributes};
}
Response_Get_Job_Attributes::Response_Get_Job_Attributes()
    : Response(Operation::Get_Job_Attributes) {}
std::vector<Group*> Response_Get_Job_Attributes::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
std::vector<Attribute*>
Response_Get_Job_Attributes::G_job_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &copies,
          &copies_actual,
          &cover_back,
          &cover_back_actual,
          &cover_front,
          &cover_front_actual,
          &current_page_order,
          &date_time_at_completed,
          &date_time_at_creation,
          &date_time_at_processing,
          &document_charset_supplied,
          &document_format_details_supplied,
          &document_format_ready,
          &document_format_supplied,
          &document_format_version_supplied,
          &document_message_supplied,
          &document_metadata,
          &document_name_supplied,
          &document_natural_language_supplied,
          &errors_count,
          &feed_orientation,
          &finishings,
          &finishings_col,
          &finishings_col_actual,
          &font_name_requested,
          &font_size_requested,
          &force_front_side,
          &force_front_side_actual,
          &imposition_template,
          &impressions_completed_current_copy,
          &insert_sheet,
          &insert_sheet_actual,
          &job_account_id,
          &job_account_id_actual,
          &job_account_type,
          &job_accounting_sheets,
          &job_accounting_sheets_actual,
          &job_accounting_user_id,
          &job_accounting_user_id_actual,
          &job_attribute_fidelity,
          &job_charge_info,
          &job_collation_type,
          &job_copies,
          &job_copies_actual,
          &job_cover_back,
          &job_cover_back_actual,
          &job_cover_front,
          &job_cover_front_actual,
          &job_delay_output_until,
          &job_delay_output_until_time,
          &job_detailed_status_messages,
          &job_document_access_errors,
          &job_error_action,
          &job_error_sheet,
          &job_error_sheet_actual,
          &job_finishings,
          &job_finishings_col,
          &job_finishings_col_actual,
          &job_hold_until,
          &job_hold_until_time,
          &job_id,
          &job_impressions,
          &job_impressions_completed,
          &job_k_octets,
          &job_k_octets_processed,
          &job_mandatory_attributes,
          &job_media_sheets,
          &job_media_sheets_completed,
          &job_message_from_operator,
          &job_message_to_operator,
          &job_message_to_operator_actual,
          &job_more_info,
          &job_name,
          &job_originating_user_name,
          &job_originating_user_uri,
          &job_pages,
          &job_pages_completed,
          &job_pages_completed_current_copy,
          &job_pages_per_set,
          &job_phone_number,
          &job_printer_up_time,
          &job_printer_uri,
          &job_priority,
          &job_priority_actual,
          &job_recipient_name,
          &job_resource_ids,
          &job_save_disposition,
          &job_save_printer_make_and_model,
          &job_sheet_message,
          &job_sheet_message_actual,
          &job_sheets,
          &job_sheets_col,
          &job_sheets_col_actual,
          &job_state,
          &job_state_message,
          &job_state_reasons,
          &job_uri,
          &job_uuid,
          &media,
          &media_col,
          &media_col_actual,
          &media_input_tray_check,
          &multiple_document_handling,
          &number_of_documents,
          &number_of_intervening_jobs,
          &number_up,
          &number_up_actual,
          &orientation_requested,
          &original_requesting_user_name,
          &output_bin,
          &output_device,
          &output_device_actual,
          &output_device_assigned,
          &output_device_job_state_message,
          &output_device_uuid_assigned,
          &overrides,
          &overrides_actual,
          &page_delivery,
          &page_order_received,
          &page_ranges,
          &page_ranges_actual,
          &pages_per_subset,
          &pdl_init_file,
          &presentation_direction_number_up,
          &print_color_mode,
          &print_content_optimize,
          &print_quality,
          &print_rendering_intent,
          &print_scaling,
          &printer_resolution,
          &printer_resolution_actual,
          &proof_print,
          &separator_sheets,
          &separator_sheets_actual,
          &sheet_collate,
          &sheet_completed_copy_number,
          &sheet_completed_document_number,
          &sides,
          &time_at_completed,
          &time_at_creation,
          &time_at_processing,
          &warnings_count,
          &x_image_position,
          &x_image_shift,
          &x_image_shift_actual,
          &x_side1_image_shift,
          &x_side1_image_shift_actual,
          &x_side2_image_shift,
          &x_side2_image_shift_actual,
          &y_image_position,
          &y_image_shift,
          &y_image_shift_actual,
          &y_side1_image_shift,
          &y_side1_image_shift_actual,
          &y_side2_image_shift,
          &y_side2_image_shift_actual};
}
Request_Get_Jobs::Request_Get_Jobs() : Request(Operation::Get_Jobs) {}
std::vector<Group*> Request_Get_Jobs::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Get_Jobs::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &requesting_user_name,
          &limit,
          &requested_attributes,
          &which_jobs,
          &my_jobs};
}
Response_Get_Jobs::Response_Get_Jobs() : Response(Operation::Get_Jobs) {}
std::vector<Group*> Response_Get_Jobs::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
Request_Get_Printer_Attributes::Request_Get_Printer_Attributes()
    : Request(Operation::Get_Printer_Attributes) {}
std::vector<Group*> Request_Get_Printer_Attributes::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Get_Printer_Attributes::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,   &attributes_natural_language,
          &printer_uri,          &requesting_user_name,
          &requested_attributes, &document_format};
}
Response_Get_Printer_Attributes::Response_Get_Printer_Attributes()
    : Response(Operation::Get_Printer_Attributes) {}
std::vector<Group*> Response_Get_Printer_Attributes::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &printer_attributes};
}
std::vector<Attribute*>
Response_Get_Printer_Attributes::G_printer_attributes::GetKnownAttributes() {
  return {&baling_type_supported,
          &baling_when_supported,
          &binding_reference_edge_supported,
          &binding_type_supported,
          &charset_configured,
          &charset_supported,
          &coating_sides_supported,
          &coating_type_supported,
          &color_supported,
          &compression_supported,
          &copies_default,
          &copies_supported,
          &cover_back_default,
          &cover_back_supported,
          &cover_front_default,
          &cover_front_supported,
          &covering_name_supported,
          &device_service_count,
          &device_uuid,
          &document_charset_default,
          &document_charset_supported,
          &document_digital_signature_default,
          &document_digital_signature_supported,
          &document_format_default,
          &document_format_details_default,
          &document_format_details_supported,
          &document_format_supported,
          &document_format_varying_attributes,
          &document_format_version_default,
          &document_format_version_supported,
          &document_natural_language_default,
          &document_natural_language_supported,
          &document_password_supported,
          &feed_orientation_supported,
          &finishing_template_supported,
          &finishings_col_database,
          &finishings_col_default,
          &finishings_col_ready,
          &finishings_default,
          &finishings_ready,
          &finishings_supported,
          &folding_direction_supported,
          &folding_offset_supported,
          &folding_reference_edge_supported,
          &font_name_requested_default,
          &font_name_requested_supported,
          &font_size_requested_default,
          &font_size_requested_supported,
          &generated_natural_language_supported,
          &identify_actions_default,
          &identify_actions_supported,
          &insert_after_page_number_supported,
          &insert_count_supported,
          &insert_sheet_default,
          &ipp_features_supported,
          &ipp_versions_supported,
          &ippget_event_life,
          &job_account_id_default,
          &job_account_id_supported,
          &job_account_type_default,
          &job_account_type_supported,
          &job_accounting_sheets_default,
          &job_accounting_user_id_default,
          &job_accounting_user_id_supported,
          &job_authorization_uri_supported,
          &job_constraints_supported,
          &job_copies_default,
          &job_copies_supported,
          &job_cover_back_default,
          &job_cover_back_supported,
          &job_cover_front_default,
          &job_cover_front_supported,
          &job_delay_output_until_default,
          &job_delay_output_until_supported,
          &job_delay_output_until_time_supported,
          &job_error_action_default,
          &job_error_action_supported,
          &job_error_sheet_default,
          &job_finishings_col_default,
          &job_finishings_col_ready,
          &job_finishings_default,
          &job_finishings_ready,
          &job_finishings_supported,
          &job_hold_until_default,
          &job_hold_until_supported,
          &job_hold_until_time_supported,
          &job_ids_supported,
          &job_impressions_supported,
          &job_k_octets_supported,
          &job_media_sheets_supported,
          &job_message_to_operator_default,
          &job_message_to_operator_supported,
          &job_pages_per_set_supported,
          &job_password_encryption_supported,
          &job_password_supported,
          &job_phone_number_default,
          &job_phone_number_supported,
          &job_priority_default,
          &job_priority_supported,
          &job_recipient_name_default,
          &job_recipient_name_supported,
          &job_resolvers_supported,
          &job_sheet_message_default,
          &job_sheet_message_supported,
          &job_sheets_col_default,
          &job_sheets_default,
          &job_sheets_supported,
          &job_spooling_supported,
          &jpeg_k_octets_supported,
          &jpeg_x_dimension_supported,
          &jpeg_y_dimension_supported,
          &laminating_sides_supported,
          &laminating_type_supported,
          &max_save_info_supported,
          &max_stitching_locations_supported,
          &media_back_coating_supported,
          &media_bottom_margin_supported,
          &media_col_database,
          &media_col_default,
          &media_col_ready,
          &media_color_supported,
          &media_default,
          &media_front_coating_supported,
          &media_grain_supported,
          &media_hole_count_supported,
          &media_info_supported,
          &media_left_margin_supported,
          &media_order_count_supported,
          &media_pre_printed_supported,
          &media_ready,
          &media_recycled_supported,
          &media_right_margin_supported,
          &media_size_supported,
          &media_source_supported,
          &media_supported,
          &media_thickness_supported,
          &media_tooth_supported,
          &media_top_margin_supported,
          &media_type_supported,
          &media_weight_metric_supported,
          &multiple_document_handling_default,
          &multiple_document_handling_supported,
          &multiple_document_jobs_supported,
          &multiple_operation_time_out,
          &multiple_operation_time_out_action,
          &natural_language_configured,
          &notify_events_default,
          &notify_events_supported,
          &notify_lease_duration_default,
          &notify_lease_duration_supported,
          &notify_pull_method_supported,
          &notify_schemes_supported,
          &number_up_default,
          &number_up_supported,
          &oauth_authorization_server_uri,
          &operations_supported,
          &orientation_requested_default,
          &orientation_requested_supported,
          &output_bin_default,
          &output_bin_supported,
          &output_device_supported,
          &output_device_uuid_supported,
          &page_delivery_default,
          &page_delivery_supported,
          &page_order_received_default,
          &page_order_received_supported,
          &page_ranges_supported,
          &pages_per_minute,
          &pages_per_minute_color,
          &pages_per_subset_supported,
          &parent_printers_supported,
          &pdf_k_octets_supported,
          &pdf_versions_supported,
          &pdl_init_file_default,
          &pdl_init_file_entry_supported,
          &pdl_init_file_location_supported,
          &pdl_init_file_name_subdirectory_supported,
          &pdl_init_file_name_supported,
          &pdl_init_file_supported,
          &pdl_override_supported,
          &preferred_attributes_supported,
          &presentation_direction_number_up_default,
          &presentation_direction_number_up_supported,
          &print_color_mode_default,
          &print_color_mode_supported,
          &print_content_optimize_default,
          &print_content_optimize_supported,
          &print_quality_default,
          &print_quality_supported,
          &print_rendering_intent_default,
          &print_rendering_intent_supported,
          &printer_alert,
          &printer_alert_description,
          &printer_charge_info,
          &printer_charge_info_uri,
          &printer_config_change_date_time,
          &printer_config_change_time,
          &printer_config_changes,
          &printer_contact_col,
          &printer_current_time,
          &printer_detailed_status_messages,
          &printer_device_id,
          &printer_dns_sd_name,
          &printer_driver_installer,
          &printer_finisher,
          &printer_finisher_description,
          &printer_finisher_supplies,
          &printer_finisher_supplies_description,
          &printer_geo_location,
          &printer_icc_profiles,
          &printer_icons,
          &printer_id,
          &printer_impressions_completed,
          &printer_info,
          &printer_input_tray,
          &printer_is_accepting_jobs,
          &printer_location,
          &printer_make_and_model,
          &printer_media_sheets_completed,
          &printer_message_date_time,
          &printer_message_from_operator,
          &printer_message_time,
          &printer_more_info,
          &printer_more_info_manufacturer,
          &printer_name,
          &printer_organization,
          &printer_organizational_unit,
          &printer_output_tray,
          &printer_pages_completed,
          &printer_resolution_default,
          &printer_resolution_supported,
          &printer_state,
          &printer_state_change_date_time,
          &printer_state_change_time,
          &printer_state_message,
          &printer_state_reasons,
          &printer_static_resource_directory_uri,
          &printer_static_resource_k_octets_free,
          &printer_static_resource_k_octets_supported,
          &printer_strings_languages_supported,
          &printer_strings_uri,
          &printer_supply,
          &printer_supply_description,
          &printer_supply_info_uri,
          &printer_up_time,
          &printer_uri_supported,
          &printer_uuid,
          &printer_xri_supported,
          &proof_print_default,
          &proof_print_supported,
          &punching_hole_diameter_configured,
          &punching_locations_supported,
          &punching_offset_supported,
          &punching_reference_edge_supported,
          &pwg_raster_document_resolution_supported,
          &pwg_raster_document_sheet_back,
          &pwg_raster_document_type_supported,
          &queued_job_count,
          &reference_uri_schemes_supported,
          &requesting_user_uri_supported,
          &save_disposition_supported,
          &save_document_format_default,
          &save_document_format_supported,
          &save_location_default,
          &save_location_supported,
          &save_name_subdirectory_supported,
          &save_name_supported,
          &separator_sheets_default,
          &sheet_collate_default,
          &sheet_collate_supported,
          &sides_default,
          &sides_supported,
          &stitching_angle_supported,
          &stitching_locations_supported,
          &stitching_method_supported,
          &stitching_offset_supported,
          &stitching_reference_edge_supported,
          &subordinate_printers_supported,
          &trimming_offset_supported,
          &trimming_reference_edge_supported,
          &trimming_type_supported,
          &trimming_when_supported,
          &uri_authentication_supported,
          &uri_security_supported,
          &which_jobs_supported,
          &x_image_position_default,
          &x_image_position_supported,
          &x_image_shift_default,
          &x_image_shift_supported,
          &x_side1_image_shift_default,
          &x_side1_image_shift_supported,
          &x_side2_image_shift_default,
          &x_side2_image_shift_supported,
          &xri_authentication_supported,
          &xri_security_supported,
          &xri_uri_scheme_supported,
          &y_image_position_default,
          &y_image_position_supported,
          &y_image_shift_default,
          &y_image_shift_supported,
          &y_side1_image_shift_default,
          &y_side1_image_shift_supported,
          &y_side2_image_shift_default,
          &y_side2_image_shift_supported};
}
Request_Hold_Job::Request_Hold_Job() : Request(Operation::Hold_Job) {}
std::vector<Group*> Request_Hold_Job::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Hold_Job::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &requesting_user_name,
          &message,
          &job_hold_until};
}
Response_Hold_Job::Response_Hold_Job() : Response(Operation::Hold_Job) {}
std::vector<Group*> Response_Hold_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
Request_Pause_Printer::Request_Pause_Printer()
    : Request(Operation::Pause_Printer) {}
std::vector<Group*> Request_Pause_Printer::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Pause_Printer::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language, &printer_uri,
          &requesting_user_name};
}
Response_Pause_Printer::Response_Pause_Printer()
    : Response(Operation::Pause_Printer) {}
std::vector<Group*> Response_Pause_Printer::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
Request_Print_Job::Request_Print_Job() : Request(Operation::Print_Job) {}
std::vector<Group*> Request_Print_Job::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
std::vector<Attribute*>
Request_Print_Job::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language,
          &printer_uri,        &requesting_user_name,
          &job_name,           &ipp_attribute_fidelity,
          &document_name,      &compression,
          &document_format,    &document_natural_language,
          &job_k_octets,       &job_impressions,
          &job_media_sheets};
}
Response_Print_Job::Response_Print_Job() : Response(Operation::Print_Job) {}
std::vector<Group*> Response_Print_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
Request_Print_URI::Request_Print_URI() : Request(Operation::Print_URI) {}
std::vector<Group*> Request_Print_URI::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
std::vector<Attribute*>
Request_Print_URI::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &document_uri,
          &requesting_user_name,
          &job_name,
          &ipp_attribute_fidelity,
          &document_name,
          &compression,
          &document_format,
          &document_natural_language,
          &job_k_octets,
          &job_impressions,
          &job_media_sheets};
}
Response_Print_URI::Response_Print_URI() : Response(Operation::Print_URI) {}
std::vector<Group*> Response_Print_URI::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
std::vector<Attribute*>
Response_Print_URI::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset, &attributes_natural_language, &status_message,
          &detailed_status_message, &document_access_error};
}
Request_Release_Job::Request_Release_Job() : Request(Operation::Release_Job) {}
std::vector<Group*> Request_Release_Job::GetKnownGroups() {
  return {&operation_attributes};
}
Response_Release_Job::Response_Release_Job()
    : Response(Operation::Release_Job) {}
std::vector<Group*> Response_Release_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
Request_Resume_Printer::Request_Resume_Printer()
    : Request(Operation::Resume_Printer) {}
std::vector<Group*> Request_Resume_Printer::GetKnownGroups() {
  return {&operation_attributes};
}
Response_Resume_Printer::Response_Resume_Printer()
    : Response(Operation::Resume_Printer) {}
std::vector<Group*> Response_Resume_Printer::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
Request_Send_Document::Request_Send_Document()
    : Request(Operation::Send_Document) {}
std::vector<Group*> Request_Send_Document::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Send_Document::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &requesting_user_name,
          &document_name,
          &compression,
          &document_format,
          &document_natural_language,
          &last_document};
}
Response_Send_Document::Response_Send_Document()
    : Response(Operation::Send_Document) {}
std::vector<Group*> Response_Send_Document::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
Request_Send_URI::Request_Send_URI() : Request(Operation::Send_URI) {}
std::vector<Group*> Request_Send_URI::GetKnownGroups() {
  return {&operation_attributes};
}
std::vector<Attribute*>
Request_Send_URI::G_operation_attributes::GetKnownAttributes() {
  return {&attributes_charset,
          &attributes_natural_language,
          &printer_uri,
          &job_id,
          &job_uri,
          &requesting_user_name,
          &document_name,
          &compression,
          &document_format,
          &document_natural_language,
          &last_document,
          &document_uri};
}
Response_Send_URI::Response_Send_URI() : Response(Operation::Send_URI) {}
std::vector<Group*> Response_Send_URI::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes, &job_attributes};
}
Request_Validate_Job::Request_Validate_Job()
    : Request(Operation::Validate_Job) {}
std::vector<Group*> Request_Validate_Job::GetKnownGroups() {
  return {&operation_attributes, &job_attributes};
}
Response_Validate_Job::Response_Validate_Job()
    : Response(Operation::Validate_Job) {}
std::vector<Group*> Response_Validate_Job::GetKnownGroups() {
  return {&operation_attributes, &unsupported_attributes};
}
}  // namespace ipp
