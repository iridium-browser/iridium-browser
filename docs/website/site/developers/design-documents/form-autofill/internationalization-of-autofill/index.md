---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/form-autofill
  - Form Autofill
page_name: internationalization-of-autofill
title: Internationalization of Autofill
---

**Objective**
To extend the Autofill feature in Chrome to better handle non-English and non-US
pages and user address and credit card information.

## Background

The Autofill feature shipped with Chrome 6. Localization of the Autofill UI was
achieved, but opportunities to provide deeper internationalization of the
feature exist.

User Experience

**Addresses**

The addresses stored in Autofill settings/preferences will better adapt to
non-US addresses. For example, a Canadian address will present "Postal Code"
instead of "Zip Code". Additional fields such as "Prefecture" may be appropriate
in Japan.

The "Country" field will use a popup selection of countries. This allows for
more dynamic structure for locale-specific address information.

> > [<img alt="image"
> > src="/developers/design-documents/form-autofill/internationalization-of-autofill/autofill_i18l_address_ui.jpg">](/developers/design-documents/form-autofill/internationalization-of-autofill/autofill_i18l_address_ui.jpg)

For different regions, such as Japan, additional local-specific fields may be
introduced. For example, notice "Prefecture" below. Notice also the renaming of
"Zip code" to "Postal code" and reordering of fields:

> > [<img alt="image"
> > src="/developers/design-documents/form-autofill/internationalization-of-autofill/autofill_i18l_address_jp_ui.jpg">](/developers/design-documents/form-autofill/internationalization-of-autofill/autofill_i18l_address_jp_ui.jpg)

**Form Field Matching**

Autofill analyses web pages to find appropriate form fields for filling. This
analysis will be enhanced to cover a greater variety of non-English pages.

**HTML / JavaScript based UI**

Platform-specific UI for Autofill will be deprecated in favor of HTML /
JavaScript based UI (aka "DOMUI"). Internationalization of Autofill will apply
to this new DOMUI only. Dynamic layout of the Autofill user preferences will be
data-driven based on declarative descriptions of locale-specific address
formats.

**Implementation**
**Data Structures**

****Locale information is sourced from three places:****

1.  Application Locale. The locale of the running Chrome application.
2.  Form Locale. The locale information, if any, associated with the
            HTML page containing the form.
3.  Address Locale. The locale dictated by the address "Country" field
            set by the user in Autofill preferences.

**Program Flow**

**During page-load Autofill analyses the forms within the page for fillable
fields. Program flow (A) is the current flow of control through the system.**

**Forms may or may not contain locale information directly, in which case we
attempt to derive the locale by other means. The strategy employed is as
follows:**

**1.**

**********The FormManager will be enhanced to pull locale information from the
WebKit DOM (see (B) in the diagram below).**********

**Give preference to local information set in the HTML markup.****

****For example:******

**&lt;form lang="fr-CA"&gt;...&lt;/form&gt;**

**&lt;div lang="fr-CA"&gt;&lt;form&gt;...&lt;/form&gt;&lt;/div&gt;**

**&lt;html&gt;&lt;head&gt;&lt;meta http-equiv="Content-Language"
content="fr-CA"/&gt;&lt;/head&gt;...&lt;/html&gt;**

**Would all yield a Form Locale of "fr-CA".**

**2. If no locale information is set by the page author, then the textual contents of the page are analysed.**

****The Compact Language Detection ( src/third_party/cld/... ) module is
utilized for this****

**********(see (C) in diagram below)************

****.******

**3.**

******URL top-level domain suffix may also be used if other sources of
information about locale are not present (not pictured).******

<img alt="image"
src="/developers/design-documents/form-autofill/internationalization-of-autofill/autofill_i18n_design.jpg">
