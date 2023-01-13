---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/web-platform-status
  - Web Platform Status
page_name: forms
title: HTML5 Forms Status
---

Dev Contact: tkent@

Updated: October 4, 2013

Note that this page doesn't cover iOS version of Google Chrome because it uses
WebKit shipped in iOS.

## On stable release

\[M5\] indicates a feature which is available since Milestone 5, a.k.a. Google
Chrome 5.0. \[M6\] for Google Chrome 6.0.

### Common features

*   autofocus attribute (autofocus property) of
            input/textarea/select/keygen elements
*   labels property of form control elements \[M6\]
*   willValidate property
*   validity property
    *   typeMismatch for &lt;input type=email&gt; and &lt;input
                type=url&gt;
    *   valueMissing and required attribute (required property)
    *   patternMismatch and pattern attribute (pattern property)
    *   tooLong
        *   tooLong behavior was updated in \[M13\]. Values set by a
                    script won't be too long.
    *   rangeOverflow and max attribute (max property) \[M5\]
    *   rangeUnderflow and min attribute (min property) \[M5\]
    *   stepMismatch and step attribute (step property) \[M5\]
    *   badInput support for number, date, datetime-local, month, time,
                week. \[M25\]
    *   valid as a combination of the above
*   checkValidity() and 'invalid' event
*   validationMessage property \[M10\]
*   :default, :valid, :invalid, :optional, and :required CSS selectors
    *   form element doesn't support :valid and :invalid yet.
*   form, formaction, formenctype, formmethod, formtarget attributes and
            the corresponding properties \[M9\]
    Note: form property has been supported for a long time though form attribute
    was not supported.
    *   formMethod / formEnctype properties of input element, and
                formMethod / formEnctype properties of button element correctly
                return normalized values. \[M16\]
    *   formMethod property returns empty string if formmethod attribute
                value is missing. \[M26\]
    *   formEnctype property returns empty string if formenctype
                attribute value is missing. \[M26\]

*   'formchange' and 'forminput' events were introduce in \[M10\], and
            removed since \[M12\].

### Form element

*   Interactive validation \[M5\]
    Invalid form controls prevent the form submission, and show a validation
    message.
    formnovalidate attribute (formNoValidate property) of submit button is
    supported.
    Note: In Google Chrome 6, the interactive validation feature worked only for
    documents with &lt;!DOCTYPE html&gt;. \[M6\]
    Note: In Google Chrome 7, the interactive validation feature was disabled.
    \[M7\]
    Note: In Google Chrome 10, the interactive validation feature was enabled
    with a bubble UI. \[M10\]
*   enctype="text/plain" support \[M17\]
*   method / enctype properties correctly return normalized values.
            \[M16\]
*   RadioNodeList support in HTMLFormElement::elements \[M21\]

### Fieldset element

*   name and type properties \[M19\]
*   disabled attribute (disabled property) \[M20\]
*   elements property \[M21\]

### Label element

*   control property of label element \[M6\]
*   form attribute \[M19\]

### Input element

*   multiple attribute (multiple DOM property) of &lt;input
            type=file&gt; and &lt;input type=email&gt;
    *   &lt;input type=email multiple&gt; allows whitespace around ','
                since \[M14\]
*   placeholder attribute (placeholder property)
    *   The behavior of placeholder attribute was changed since \[M17\].
                Placeholder text is shown until a user enters the first letter.
*   accept attribute for &lt;input type=file&gt; \[M8\]
    *   File name suffix support \[M21\]
*   files property
    *   files property is writable. \[M21\]
*   dirname attribute of input element \[M17\]
*   Support for &lt;input type=hidden name=_charset_&gt; \[M17\]
*   Dedicated UI for &lt;input type=search&gt;
*   Dedicated UI for &lt;input type=range&gt; (only horizontal)
*   type attribute (type property) recognizes email, tel, and url
    *   The UI is same as type=text.
*   &lt;input type=number&gt; with spin-button UI \[M6\]
    *   Supports wheel events \[M7\]
    *   Supports auto-repeat by mouse press since \[M7\]
    *   Localization of &lt;input type=number&gt; \[M11\]
    *   Inserting thousand-separator such as ',' in &lt;input
                type=number&gt; was disabled since \[M14\]
    *   The default width of a number type field with finite
                min/max/step values is fit to possible maximum digits. \[M15\]
*   &lt;input type=color&gt; for Windows, OS X, and Linux \[M20\]
    *   ChromeOS \[M22\]
*   &lt;input type=date&gt; for Windows, OS X, Linux, and Chrome OS
            \[M20\]
    ~~Down arrow key to open the calendar popup.~~
    Keyboard bindings in the calendar popup:
    <table>
    <tr>
    m <td>Next month</td>
    </tr>
    <tr>
    Shift + m <td>Previous month</td>
    </tr>
    <tr>
    y <td>Next year</td>
    </tr>
    <tr>
    Shift + y <td>Previous year</td>
    </tr>
    <tr>
    d <td>10 years later</td>
    </tr>
    <tr>
    Shift + d <td>10 years past</td>
    </tr>
    <tr>
    Enter <td>Set the selected date to the field, and close the popup </td>
    </tr>
    <tr>
    t <td>Select today</td>
    </tr>
    <tr>
    ESC <td>Close the popup</td>
    </tr>
    </table>
    *   Update &lt;input type=date&gt; UI for non-Android \[M24\]
        It contains multiple editable fields and a spin button.
        Key binding to open a calendar picker was changed: down arrow → Alt
        (option) + down arrow
    *   Android supports it since \[M18\]
    *   Major calendar UI update \[M27\]
*   &lt;input type=time&gt; \[M23\]
    *   Android supports it since \[M18\]
*   &lt;input type=month&gt; \[M25\]
    *   Android supports it since \[M18\]
*   &lt;input type=week&gt; for non-Android \[M25\]
*   &lt;input type=datetime-local&gt; \[M25\]
    *   Android supports it since \[M18\]
*   list attribute, and list property
    *   text, search, url, tel, and number types. \[M20\]
    *   datalist element support for &lt;input type=email&gt; \[M21\]
    *   datalist element support for &lt;input type=range&gt; \[M22\]
        *   Specified values are shown as tick marks, and the slider
                    thumb snaps on them.
    *   datalist element support for &lt;input type=color&gt; \[M22\]
    *   datalist support for date, and time \[M24\]
    *   datalist support for datetime-local, month, and week \[M25\]
    *   For unsupported input types, list property returns null.
    *   Chrome for Android doesn't support list attribute for all input
                types.
*   min/max/step/value attribute values for numeric types accepts number
            strings starting decimal points since \[M17\]
    *   ".25" was an invalid number string until \[M16\], and it's valid
                since \[M17\].
*   stepUp() and stepDown() \[M5\]
*   valueAsDate property \[M5\]
*   valueAsNumber property \[M5\]
*   Removed labels property support of &lt;input type=hidden&gt; \[M19\]
*   Indeterminate checkbox \[M6\]
*   :out-of-range and :in-range CSS selectors \[M10\]
*   height and width properties of input element \[M21\]
*   setRangeText() \[M24\]

*   type attribute of input element recognized date, datetime,
            datetime-local, month, time, and week during \[M5\] to \[M15\]
    They were disabled since \[M16\] because of their incompleteness and UI
    development

### Button element

*   type property is writable. \[M21\]

### Select element

*   validity.valueMissing and required attribute (required property)
            \[M10\]
*   selectedOptions property \[M22\]

### Datalist element

*   Introduced in \[M20\] with &lt;input list=id&gt; support

### Textarea element

*   wrap property \[M16\]
*   placeholder attribute (placeholder property)
    *   The behavior of placeholder attribute was changed since \[M17\].
                Placeholder text is shown until a user enters the first letter.
*   maxlength attribute (maxLength property)
*   setRangeText() \[M24\]

### Output element

*   Introduced in \[M9\]
*   labels property \[M19\]

### Progress element

*   Introduced in \[M6\]
*   Removed form property \[M19\]

### Meter element

*   Horizontal meter element was introduced in \[M6\]
*   Removed form property \[M19\]

---

## Milestone 29

*   IDN support for type=email
*   &lt;input type=color&gt; for Android

## Milestone 30

*   &lt;input type=week&gt; for Android
*   Buttons, input\[type=range\], checkboxes, and radio buttons get
            focus by mouse press.

## Milestone 33

*   Step base detection takes account of value attribute value.

## Not available yet

*   datalist support for Android \[in-progress\]
*   inputmode attribute for input and textarea elements \[in-progress\]
*   Dedicated UIs for datetime type with datalist support
*   type=number value range update (float → double)
*   New behavior of stepUp() and stepDown()
*   Newlines for textarea placeholder
*   reportValidity()
*   minlength and validity.tooShort
*   New behavior of min &gt; max case for type=time
*   :valid and :invalid CSS selectors for form and fieldset element
*   Automatic switching to vertical range, progress, and meter.
*   Form controls in vertical writing mode
*   CSS Selectors4: :user-error pseudo class
*   :indeterminate pseudo class for radio buttons
