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

You can help ensure that browsers' and extensions' password management
functionality can understand your site's sign-up, sign-in and change-password
forms by enriching your HTML with a dash of metadata. In particular:

1.  Add an
            `[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)`
            attribute with a value of `username` for usernames.
2.  If you've implemented an "[email
            first](https://developers.google.com/identity/toolkit/web/account-chooser#email_first)"
            sign-in flow that separates the username and password into two
            separate forms, include a form field containing the username in the
            form used to collect the password. You can, of course, hide this
            field via CSS if that's appropriate for your layout.
3.  Add an
            `[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)`
            attribute with a value of `current-password` for the password field
            on a sign-in form.
4.  Add an
            `[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)`
            attribute with a value of `new-password` for the password field on
            sign-up and change-password forms.
5.  If you require the user to type their password twice during sign-up
            or password update, add the `new-password`
            `[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)`
            attribute on both fields.

#### Sign-in Form:

&lt;form id="login" action="login.php" method="post"&gt; &lt;input type="text"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="username"**&gt;
&lt;input type="password"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="current-password"**&gt;
&lt;input type="submit" value="Sign In!"&gt; &lt;/form&gt;

#### Email First Sign-in Flow:

Collect the email: &lt;form id="login" action="login.php" method="post"&gt;
&lt;input type="text"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="username"**&gt;
&lt;input type="submit" value="Sign In!"&gt; &lt;/form&gt; Then collect the
password, but include the email as the value of a hidden form field:
&lt;style&gt; #emailfield { display: none; } &lt;/style&gt; &lt;form id="login"
action="login.php" method="post"&gt; &lt;input id="emailfield" type="text"
**value="me@example.test"**
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="username"**&gt;
&lt;input type="password"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="current-password"**&gt;
&lt;input type="submit" value="Sign In!"&gt; &lt;/form&gt;

#### Sign-up Form:

&lt;form id="login" action="signup.php" method="post"&gt; &lt;input type="text"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="username"**&gt;
&lt;input type="password"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="new-password"**&gt;
&lt;input type="submit" value="Sign Up!"&gt; &lt;/form&gt; Or: &lt;form
id="login" action="signup.php" method="post"&gt; &lt;input type="text"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="username"**&gt;
&lt;input type="password"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="new-password"**&gt;
&lt;input type="password"
**[autocomplete](https://html.spec.whatwg.org/multipage/forms.html#autofilling-form-controls:-the-autocomplete-attribute)="new-password"**&gt;
&lt;input type="submit" value="Sign Up!"&gt; &lt;/form&gt;

### Related advice

Further tips beyond just autocomplete attributes are listed in the [Create
Amazing Password Forms
page](/developers/design-documents/create-amazing-password-forms).

There are also useful [autocomplete attribute values for annotating address
forms](https://developers.google.com/web/updates/2015/06/checkout-faster-with-autofill).