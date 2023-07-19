---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: create-amazing-password-forms
title: Create Amazing Password Forms
---

Web browsers (and other agents, such as password managers) try to make the process of filling out forms as convenient to users as possible, to save time and frustration. However good they are at interpreting web pages, however, there are always a few things you, as a web developer, can make sure to do, to ensure the best experience for your users, by making your web pages accessible.

## Group related fields in a single form

Make sure that each authentication process (registration, login or change password) is grouped together in a single &lt;form&gt; element. Don’t combine multiple processes (like registration and login), or skip the &lt;form&gt; element.

## Use autocomplete attributes

[Autocomplete](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#autofilling-form-controls%3A-the-autocomplete-attribute) attributes help password managers to infer the purpose of a field in a form, saving them from accidentally saving or autofilling the wrong data. A little annotation can go a long way: some common values for text fields are “username”, “current-password” (login forms and change password forms) and “new-password” (registration and change password forms). See a [detailed write-up](/developers/design-documents/form-styles-that-chromium-understands) with examples.

Fields that are not passwords, but should be obscured, such as credit card numbers, may also have a type="password" attribute, but should contain the relevant autocomplete attribute, such as "cc-number" or "cc-csc".

## Make sure form submission is clear

Password managers need some indication that a form has been submitted. Try to make sure your web page does one of the following on form submission:

*   Performs a navigation to another page.
*   Emulates a navigation with [History.pushState or
    History.replaceState](https://developer.mozilla.org/en-US/docs/Web/API/History_API),
    and removes the password form completely.**

In addition, if you perform an asynchronous request with [XMLHttpRequest](https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest) or [fetch](https://developers.google.com/web/updates/2015/03/introduction-to-fetch), make sure that the success status is correctly set in the response headers.

## Use hidden fields for implicit information

On some forms, it is not necessary to include some information, which might be obvious from the context (for example, the username field in a change password form). However, this information is still useful to password managers (there may be multiple accounts associated with a web site, for example), so include a hidden input field containing this information even if it is not directly necessary for your form.

## Don’t try to fool the browser

Password managers (either built into the browser, or external) are designed to ease the user experience. Inserting fake fields, using incorrect autocomplete attributes or taking advantage of the weaknesses of the existing password managers simply leads to frustrated users.

## Follow existing conventions

Being different just for the sake of it is not worth it. This will lead to a poor user experience all round. This means, for example, not navigating to a separate web page if a login failed: this is unexpected, and disorienting to both the user and the password manager.

## Follow HTML guidelines

Web browsers are designed with the HTML specification in mind, and going against it can lead to unexpected issues with your web page. This means:

* Element id attributes should be unique: no two elements should have the same id.
