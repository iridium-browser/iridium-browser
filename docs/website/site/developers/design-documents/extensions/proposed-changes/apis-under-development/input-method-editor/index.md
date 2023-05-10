---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: input-method-editor
title: Input Method Editor
---

Overview
The purpose of this API is to allow third parties to develop IMEs for Chrome OS
via extensions.
Use cases
This API would be used to create IMEs that aren’t included in Chrome OS. This
could include Cloud IME solutions, alternate CJK IMEs, or English IMEs and
writing assistants.
There are three basic types of input we should support:

*   Text Generators: Generates a block of text to input into a text
            window. Voice Input is an example of this.
*   Keystroke Generators: Generates distinct keystroke, which are passed
            to the active IME. Virtual Keyboard and Handwriting inputs are
            examples of this.
*   IMEs: Manages the candidate window, composition, and processes key
            events.

An input method extension can behave as any or all of these, but only a single
extension can be active as an IME at a time.
Could this API be part of the web platform?
This is a client side feature, and doesn’t make much sense to add       to HTML5
Do you expect this API to be fairly stable?
Once finalized, I don’t expect this API to change in any way that would break
backwards compatibility.

What UI does this API expose?
On ChromeOS, the user interacts with this API through the IME selection button,
and the candidate window. As such, no new UI would need to be added. To
implement this API on Windows, Mac and Linux, an IME selection method would need
to be added, and the candidate window would need to be exposed.
Candidate Window<img alt="image"
src="https://lh6.googleusercontent.com/GPuIhveTIycZtycoRX-UpMyjUtTbRagxaFBSumy_lXMm_Jk5XrwNC2c7MDZMY_dsVhIu8yqUXC62eTajN4evCvBJqAbeBa9inZc0EHPEDrEdh7AQzRbIGjlF4p5Q4bM"
height=273px; width=372px;>
Language Selection<img alt="image"
src="https://lh4.googleusercontent.com/mI9f05co5OXWt9epa7Q7dXh0jAqPwaySZ94CI_PlzahEQvMHt0uHupj_gXhIfoMK0DPd0LKqEDie3zEz_EwFbVPAYmKCMKDHsu_x4Vp5oDlGXOufP6iC1sJMF3YGomY"
height=305px; width=337px;>
How could this API be abused?
The biggest danger of this API is that it could easily be used to create key
loggers. Android deals with this issue by adding additional security dialogs to
enable third party IMEs. The danger is mitigated slightly by the fact that
password dialogs do not send their keystrokes to IME. Extensions that generate
keystrokes such as virtual keyboards will still be able to send keystrokes to
password fields and could record this data, however.
This should be protected by both the standard security warning during
installation, and an additional warning/confirmation dialog when enabled for the
first time.
How would you implement your desired features if this API didn’t exist?
There is currently no way for third parties to add IMEs to Chrome OS. Allowing
IMEs executing in any other context than sandboxed Javascript seems much more
dangerous from a security standpoint.
Are you willing and able to develop and maintain this API?
Yes.

Draft API spec
To prevent abuse of IMEs, the API will be guarded by the “ime” permission in
manifest.json.

Manifest Entries:

To use the ime.engine APIs, you need to specify an input_components entry in the
manifest. It will contain an array describing any input components in the
extension.

It should look like:

"input_components" = \[

{

"name": "My IME", // A user visible name.

"type": "ime", // The type of input component.

"id": "my_ime", // The id that will be passed to callbacks

"description": "My Input method for Japanese", // A user visible description

"language": "ja", // The primary language this IME is used for

"layouts": {"jp::jpn", "us:dvorak:eng"} // The supported keyboard layouts for
this IME

}

\]

Types:

*   MenuItem (Object)
    *   id (String)
    *   label (optional String) (default: “”)
    *   style (optional Integer) - Enum representing if this item is:
                None, Check, Radio, or a Separator. Radio buttons between
                separators are considered grouped.
    *   visible (optional Boolean) (default: true)
    *   enabled (optional Boolean) (default: true)
    *   checked (optional Boolean) (default: false)
    *   children (optional Array of MenuItem)
*   KeyboardEvent (Object) - See
            <http://www.w3.org/TR/DOM-Level-3-Events/#events-KeyboardEvent>
    *   type (String) - One of 'keyup' or 'keydown'.
    *   key (String) - Value of the key being pressed
    *   altKey (optional Boolean) - Whether or not the ALT key is
                pressed.
    *   ctrlKey (optional Boolean) - Whether or not the CTRL key is
                pressed.
    *   shiftKey (optional Boolean) - Whether or not the SHIFT key is
                pressed.
*   InputContext (Object)
    *   textID (Integer) - This is used to specify targets of text field
                operations. This ID becomes invalid as soon as onBlur is called.
    *   type (String) - Type of value this text field edits, (Text,
                Number, Password, etc)

Methods:
chrome.

****input.ime****

.setComposition**

*   Description
    *   Set the current composition. If this extension does not own the
                active IME, this fails.
*   Parameters
    *   Object
        *   contextID (Integer) - ID of the context with the composition
                    text. This is the value provided in the InputContext from
                    onFocus.
        *   text (String)
        *   selectionStart (optional Integer) - Position in the text
                    that the selection starts at.
        *   selectionEnd (optional Integer) - Position in the text that
                    the selection ends at.
        *   cursor (Integer) - Position in the text of the cursor
        *   segments (optional Array) - List of segments and their
                    associated types.
            *   Object
                *   start (Integer)
                *   end (Integer)
                *   style (String) - \[underline, doubleUnderline\] This
                            is an enum that indicates what type this segment is.
    *   callback (optional Function(Boolean success) {} ) - Called when
                the operation completes. success indicates whether the operation
                succeeded, on failure, chrome.extension.lastError is set.

chrome.

********input********

****.ime****

.clearComposition**

*   Description
    *   Clear the current composition. If this extension does not own
                the active IME, this fails.
*   Parameters
    *   Object
        *   contextID (Integer) - ID of the context with the composition
                    text. This is the value provided in the InputContext from
                    onFocus.
    *   callback (optional Function(Boolean success) {} ) - Called when
                the operation completes. success indicates whether the operation
                succeeded, on failure, chrome.extension.lastError is set.

chrome.

********input********

****.ime****

.commitText**

*   Description
    *   Commits the provided text to the current input.
*   Parameters
    *   Object
        *   contextID (Integer) - ID of the context where the text will
                    be committed.
        *   text (String)
    *   callback (optional Function(Boolean success) {}) - Called when
                the operation completes with a boolean indicating if the text
                was accepted or not. on failure, chrome.extension.lastError is
                set.

chrome.

********input********

****.ime****

.setCandidateWindowProperties**

*   Description
    *   Sets the properties of the candidate window. This fails if the
                extension doesn’t own the active IME
*   Parameters
    *   Object
        *   engineID (String)
        *   properties (Object)
            *   visible (optional Boolean) - True to show the Candidate
                        window, false to hide it.
            *   cursorVisible (optional Boolean) - True to show the
                        cursor, false to hide it.
            *   vertical (optional Boolean) - True if the candidate
                        window should be rendered vertical, false to make it
                        horizontal.
            *   pageSize (optional Integer) - The number of candidates
                        to display per page.
            *   auxiliaryText (optional String) - Text that is shown at
                        the bottom of the candidate window.
            *   auxiliaryTextVisible (optional Boolean) - True to
                        display the auxiliary text, false to hide it.
    *   callback (optional Function(Boolean success) {}) - Called when
                the operation completes.

chrome.

********input********

****.ime****

.setCandidates**

*   Description
    *   Sets the current candidate list. This fails if this extension
                doesn’t own the active IME
*   Parameters
    *   Object
        *   contextID (Integer)
        *   candidates (Array)
            *   item (Object)
                *   candidate (String)
                *   id (Integer) - User defined id that will be used to
                            identify this candidate. It must be unique in the
                            current list of candidates.
                *   label (optional String)
                *   annotation (optional String)
                *   candidates (Array of items) - Sub-candidates
    *   callback (optional Function(Boolean success) {}) - Called when
                the operation completes.

chrome.

********input********

****.ime****

.setCursorPosition**

*   Description
    *   Set the position of the cursor in the candidate window. This is
                a no-op if this extension does not own the active IME.
*   Parameters
    *   Object
        *   contextID (Integer)
        *   candidateID (Integer) - ID of the candidate to select.
    *   callback (optional Function) - Called when the operation
                completes

chrome.

********input********

****.ime****

.setMenuItems**

*   Description
    *   Adds the provided menu items to the language menu when this IME
                is active.
*   Parameters
    *   Object
        *   engineID (String) - ID of the engine to use
        *   items (array of MenuItem) - MenuItems to add. They will be
                    added in the order they exist in the array.
    *   callback (optional Function) - Called when the operation
                completes

chrome.

********input********

****.ime****

.updateMenuItems**

*   Description
    *   Updates the state of the MenuItems specified
*   Parameters
    *   Object
        *   engineID (String) - ID of the engine to use
        *   item (MenuItem or array of MenuItem) - One or more MenuItems
                    to update.
    *   callback (optional Function) - Called when the operation
                completes

chrome.

********input********

****.ime****

.sendKeyEvent**

*   Description
    *   Sends a keystroke to the system
*   Parameters
    *   Object
        *   contextID (Integer) - ID of the context with the composition
                    text.
        *   key (KeyEvent) - Key data to send
    *   callback (optional Function) - Called when the operation
                completes

Events
chrome.

********input********

****.ime****

.onActivate**

*   Description
    *   This event is sent when an IME is activated. It signals that the
                IME will be receiving onKeyPress events.
*   Parameters
    *   engineID (String) - ID of the engine receiving the event

chrome.

********input********

****.ime****

.onDeactivated**

*   Description
    *   This event is sent when an IME is deactivated. It signals that
                the IME will no longer be receiving onKeyPress events.
*   Parameters
    *   engineID (String) - ID of the engine receiving the event

chrome.

********input********

****.ime****

.onFocus**

*   Description
    *   This event is sent when focus enters a text box. It is sent to
                all extensions that are listening to this event, and enabled by
                the user.
*   Parameters
    *   context (InputContext) - an InputContext object describing the
                text field that has acquired focus.

chrome.

********input********

****.ime****

.onBlur**

*   Description
    *   This event is sent when focus leaves a text box. It is sent to
                all extensions that are listening to this event, and enabled by
                the user.
*   Parameters
    *   contextID (Integer) - the ID of the text field that has lost
                focus. The ID is invalid after this call

chrome.

********input********

****.ime****

.onInputContextUpdate**

*   Description
    *   This event is sent when the properties of the current
                InputContext change, such as the the type. It is sent to all
                extensions that are listening to this event, and enabled by the
                user.
*   Parameters
    *   context (InputContext) - an InputContext object describing the
                text field that has changed.

chrome.

********input********

****.ime****

.onKeyEvent**

*   Description
    *   This event is sent if this extension owns the active IME.
*   Parameters
    *   engineID (String) - ID of the engine receiving the event
    *   keyData (KeyEvent) - Data on the key event
*   Return value
    *   handled (Boolean) - True if the keystroke was handled, or false
                if it should be

chrome.

********input********

****.ime****

.onCandidateClicked**

*   Parameters
    *   engineID (String) - ID of the engine receiving the event
    *   candidateID (Integer) - ID of the candidate that was clicked.
    *   button (String) -\[left, right, middle\] Which mouse buttons was
                clicked.

**chrome.**

******input******

**.ime.onMenuItemActivated**

*   Parameters
    *   engineID (String) - ID of the engine receiving the event
    *   name (String) - Name of the MenuItem which was activated

**Open questions**
