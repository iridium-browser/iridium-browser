---
breadcrumbs:
  - - /developers
    - For Developers
  - - /developers/design-documents
    - Design Documents
page_name: form-styles-that-chromium-understands
title: Password Form Styles that Chromium Understands
---

### Automatically Comprehensible Password Forms

You can help ensure that browsers' and extensions' password management functionality can understand your site's sign-up, sign-in and change-password forms by enriching your HTML with a dash of metadata. In particular:


1. Add an [`autocomplete`](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-autocomplete) attribute with a value of [`username`](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-autocomplete-username) for usernames.

2. If you've implemented an "[email first](https://developers.google.com/identity/toolkit/web/account-chooser#email_first)" sign-in flow that separates the username and password into two separate forms, include a form field containing the username in the form used to collect the password. You can, of course, hide this field via CSS if that's appropriate for your layout.

3. Add an [`autocomplete`](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute) attribute with a value of [`current-password`](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-autocomplete-current-password) for the password field on a sign-in form.

4. Add an [`autocomplete`](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute) attribute with a value of [`new-password`](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-autocomplete-new-password) for the password field on sign-up and change-password forms.

5. If you require the user to type their password twice during sign-up or password update, add the [`new-password`](https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-autocomplete-new-password) [`autocomplete`](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute) attribute on both fields.

#### Sign-in Form:

```html
<form id="login" action="/login" method="post">
  <label for="username">Username</label>
  <input
    id="username"
    type="text"
    name="username"
    autocomplete="username"
    required
  >
  <label for="password">Password</label>
  <input
    id="password"
    type="password"
    name="password"
    autocomplete="current-password"
    required
  >

  <button type="submit">Sign In</button>
</form>
```

#### Email First Sign-in Flow:

Collect the email:

```html
<form id="login" action="/login" method="post">
  <label for="username">Username</label>
  <input
    id="username"
    type="email"
    name="username"
    autocomplete="username"
    required
  >
  <button type="submit">Next</button>
</form>
```

Then collect the password, but include the email as the value of a hidden form field:

```html
<style>
  #username {
    display: none;
  }
</style>
<form id="login" action="login.php" method="post">
  <!-- user invisible -->
  <input id="username" type="email" value="user@example.com">

  <label for="password">Password</label>
  <input
    id="password"
    type="password"
    name="password"
    autocomplete="current-password"
    required
  >
  <button type="submit">Sign In</button>
</form>
```

#### Sign-up Form:

```html
<form id="signup" action="/signup" method="post">
  <label for="username">Username</label>
  <input
    id="username"
    type="text"
    name="username"
    autocomplete="username"
    required
  >
  <label for="password">New password</label>
  <input
    id="password"
    type="password"
    name="password"
    autocomplete="new-password"
    required
  >
  <button type="submit">Sign In</button>
</form>
```

Or:

```html
<form id="signup" action="/signup" method="post">
  <label for="username">Username</label>
  <input
    id="username"
    type="text"
    name="username"
    autocomplete="username"
    required
  >
  <label for="password">New password</label>
  <input
    id="password"
    type="password"
    name="password"
    autocomplete="new-password"
    required
  >
  <label for="password">Confirm new password</label>
  <input
    id="password"
    type="password"
    name="password"
    autocomplete="new-password"
    required
  >
  <button type="submit">Sign In</button>
</form>
```

### Related advice

Further tips beyond just autocomplete attributes are listed in the [Create Amazing Password Forms page](/developers/design-documents/create-amazing-password-forms).

There are also useful [autocomplete attribute values for annotating address forms](https://web.dev/learn/forms/autofill/) and [Sign-in form best practice](https://web.dev/sign-in-form-best-practices/)
