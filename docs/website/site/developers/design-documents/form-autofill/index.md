---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: form-autofill
title: Form Autofill
---

**Objective**
A feature in Chrome to automatically fill out an html form with appropriate
data.

## Background

Most browsers have an autofill feature, and Google toolbar for Firefox has an
autofill feature. Every browser presents a slightly different user experience,
but the basic paradigms of how it should appear to the user are well
established.
**Other Implementations**

*   [Google
            Toolbar](http://toolbar.google.com/T5/intl/en/features.html#autofill)
*   [Firefox](http://support.mozilla.com/en-US/kb/Form+autocomplete)
*   [Safari](http://docs.info.apple.com/article.html?path=Safari/2.0/en/ibr47.html)
*   [Internet
            Explorer](http://msdn.microsoft.com/en-us/library/ms533032%28VS.85%29.aspx)

## Overview

Every time the user submits a form, entries in text input fields organized by
the name of the field specified in html get stored in the SQLite database under
the profile.
When the user opens a webpage containing a form and types into one of the
fields, Chrome checks the database for past entries into fields with the same
name. As the user types, a pop-up menu appears allowing the user to select from
entries which match the prefix already entered.

## User Experience

The user visits a website containing a form and commences to filling it out.
When the user starts filling out a field autofill kicks in and displays a menu
showing frequently entered data into fields with the same name as the currently
focused field.
**Implementation**
**Data Structures**
AutofillForm: A struct for storing name/value pairs as well as origin and action
urls. One of these gets constructed when the user submits the form. The class
uses a static factory method AutofillForm::CreateAutofillForm() which might
return NULL if the form itself is unsuitable for autofilling.
AutofillManager: The per-tab autofill manager. Handles receiving form data from
the renderer and storing and retrieving name/value pairs through WebDataService.
Each WebContents has a single AutofillManager which gets accessed by the method
GetAutofillManager().
**Program Flow**
When the user submits a form, the form gets sent to
AutofillForm::CreateAutofillForm(). If the form is suitable for autofill, the
return value will be a new AutofillForm holding all the values that the user
entered in the form. WebFrameLoaderClient::dispatchWillSubmitForm() passes the
AutofillForm as the argument of its delegate's function
OnAutofillFormSubmitted(). The delegate, provided it is a RenderView, sends a
ViewHostMsg_AutofillFormSubmitted message to its RenderViewHost. Upon getting
the message, the RenderViewHost then forwards the form to its delegate as the
argument of AutofillFormsSeen(). That delegate should be a WebContents. The
WebContents contains a pointer to its AutofillManager, and passes the form to
AutofillManager::StoreFormEntriesInWebDatabase(). The AutofillManager then uses
WebDataService to record the submitted form entries into the local database.
When the user navigates to a form that is potentially fillable by autofill,
WebFrameLoaderClient::dispatchDidFinishDocumentLoad() calls
WebFrameLoaderClient::RegisterAutofillListeners() which attaches a
webkit_glue::FormAutocompleteListener to each text input field. The
FormAutocompleteListener waits for keyboard events in the field and calls
RenderView::QueryFormFieldAutofill for suggestions. The RenderView sends a
message to its RenderViewHost which calls GetAutofillSuggestions on its
delegate. That delegate should be a WebContents.
WebContents::GetAutofillSuggestions() gets the list of suggestions from its
AutofillManager. The AutofillManager gets the suggestion list from
WebDataService, and then calls back
RenderViewHost::AutofillSuggestionsReturned().
**Tables in the SQLite Databse**
autofill: Stores in each row the following information for an entry into a form:
pair_id, name, value, count. pair_id is an integer unique to that row in the
table. pair_id is meant to be thought of as uniquely identifying a name/value
pair that has been entered. name/value pairs are only entered into one row of
the autofill table. After that if the same value is entered in a field with that
name, the count entry in the row gets incremented, and a separate row gets
entered in autofill_dates.
autofill_dates: Gets a row added for each form element (of text input type),
every time the user submits a form. Each row contains a pair_id idenitifying the
corresponding row in the autofill table and a time date_created indicating when
that form element was added.

## Internationalization

Internationalization of the Autofill feature is described
[here](/developers/design-documents/form-autofill/internationalization-of-autofill).

## Future Work

## Fill not just the currently focused form field but other fields in the form once the user has identified themselves.

Introduce a profile data structure that stores specific information by user such
as names (first/last) addresses telephone numbers, and record the type of data
that gets entered into a form rather than the data itself.
Handle names in a way that accounts for fields which ask for full name on one
line versus first and last name separately.
Handle phone number entry with "split" fields. Different sites require phone
numbers to be entered in various formats. Some require the punctuation, some
require country code, area code, prefix and extension in separate fields. Phone
numbers are formatted differently from country to country. We should add
heuristics to handle these various cases.
Allow the user to specify multiple addresses (mailing, shipping).
Just in case autofill fills in everything wrong (such as the user is borrowing
somebody's computer, and it filled in the computer's owner's information
somehow) a gentle, unassuming control appears allowing the user to turn autofill
off for the current form.
Allow multiple profiles.
Allow credit card information in a profile with extra password protection.

## Privacy and Security Concerns

Autofill should not enter information into a text field which a program could
potentially read unless the user has implicitly given consent to upload that
information such as by selecting an item from a drop-down menu or hitting the
submit button. Otherwise, a sinister person could construct a website with
hidden form fields which harvest personal data by getting autofilled.

**Subsystem design docs**

[Syncing autofill Wallet metadata](https://goo.gl/LS2y6M)
