---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: mac-accessibility
title: Mac Accessibility
---

There are two main issues to think about when writing new native Mac code: full
keyboard access and VoiceOver compatibility.

### Full keyboard access

On Mac OS X, the operating system is not fully keyboard-accessible by default.
For example, there's no way to access the menu bar or most toolbars without
using the mouse. However, users can turn on Full Keyboard Access by pressing
Control+F1, and they can separately control whether they want Tab to cycle
between all controls or just text boxes and lists by pressing Control+F7, or in
System Preferences &gt; Keyboard &gt; Keyboard Shortcuts. When Full Keyboard
Access is enabled, users can press keys like Control+F2 to highlight the menu
bar, Control+F3 to highlight the Dock, and Control+F5 to highlight an
application's toolbar.

When Full Keyboard Access is enabled, Chromium should provide full keyboard
access to all controls. In many cases, Cocoa will take care of this for you
automatically, so you should simply test its behavior. If you're implementing a
custom UI widget, you may need to implement it yourself. To determine if Full
Keyboard Access is enabled programmatically, query
[isFullKeyboardAccessEnabled](http://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ApplicationKit/Classes/NSApplication_Class/Reference/Reference.html)
in NSApplication.

### Accessibility Inspector

Xcode comes with a tool that let's you as the developer see all of the
information your new widget exposes with respect to accessibility. With the
developer tools installed, use spotlight and search for "Accessibility
Inspector", then launch it and hover over your widget to see all of the
attributes a potential assistive technology will receive. Look carefully for
errors and warnings indicated by the tool. Also, read through each of the
attributes and make sure they make sense. For further guidance, see the
NSAccessibility informal protocol reference and the brief code snippet below.
Sometimes the best way to figure out what the attributes should be is to hover
over a similar control elsewhere in Chromium or in a different application.

### VoiceOver compatibility

Mac OS X includes a built-in accessibility service called
[VoiceOver](http://www.apple.com/accessibility/voiceover/) that provides an
alternative experience for users who are blind or low-vision. VoiceOver works
quite well with Chromium today and it's important that we keep it that way!

It can take some time to master VoiceOver, but the basics aren't too hard. Press
Cmd+F5 to toggle it on or off; give it a few seconds to start up if you haven't
used it before. Consider going through the tutorial.

VoiceOver provides a synthesized-speech interface by default, but users with a
refreshable braille display can access equivalent content in braille form, too.
If you're not used to listening to synthesized speech, you can turn down the
volume and ignore it, and instead just watch the Caption Panel that shows you
everything VoiceOver is saying.

When testing with VoiceOver, it's very important that you only use VoiceOver
keys to interact. Don't test things using other keyboard shortcuts you might
know, because you won't be testing whether or not your interface is discoverable
for VoiceOver users.

The VoiceOver keys are Control+Option and are abbreviated VO. Keep your fingers
on these keys! Here are the keys you need to get started:

*   VO+Left/Right: move to the previous or next item.
*   VO+Shift+Down: start interacting with the current item
            (hierarchically).
*   VO+Shift+Up: stop interacting with the current item (hierarchically)
            - move up a level.
*   VO+Space: click

When writing custom cocoa code, the most common change you'll need to make is to
override the **title** and **role** of a UI element. The role tells VoiceOver
what type of element or control it is, and the title tells it what text to use
when describing it. You occasionally may want to just tell VoiceOver to skip an
element entirely, if it's purely decorative. Here's an example code snippet that
sets the accessible label on button **button** to the localized string
**IDS_MY_BUTTON_LABEL:**

NSString\* description = l10n_util::GetNSStringWithFixup(IDS_MY_BUTTON_LABEL);

\[button accessibilitySetOverrideValue:description

forAttribute:NSAccessibilityDescriptionAttribute\];

For detailed information, start here: [Accessibility Programming Guidelines for
Cocoa
(developer.apple.com)](http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/Accessibility/cocoaAXSupportingAttributes/cocoaAXSupportAttributes.html)
