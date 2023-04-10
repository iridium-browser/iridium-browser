---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: automaticspellingcorrection
title: Automatic Spelling Correction
---

# A new feature to add to Chrome would be automatic spelling correction. In this feature, common misspelled words will be replaced immediately with the correct word, on the fly, while typing. For example, words such as "the" typed as "teh", "more" typed as "moer", will be replaced with "the" and "more" respectively.

A natural apprehension is that spellings can be auto corrected without the user
wanting it to happen. In this particular implementation, the algorithm for
changing the word is kept stringent, so that cases with ambiguity are not dealt
with at all, thus reducing false positives. Further, this feature can be turned
OFF for usage in text box which requires technical words not necessarily in the
dictionary. Selective use of this feature will probably make daily browser
activities, such as typing an email, a better and faster experience.

A somewhat detailed design is presented below.

## Infrastructure Support

This would require adding support to Webkit. The TypingCommand class
(TypingCommand.cpp) supports a method markMisspellingsAfterTyping. This takes a
look at the selection that results after typing and determines whether
spellcheck is required. This method will ask the
markMisspellingsAfterTypingToPosition method in Editor (Editor.cpp) to provide
the integer positions: firstMisspelledCharOffset and firstMisspelledCharCount.
These positions will be required to generate the exact subrange within the
Editor object which is to be replaced with the auto correct word after typing.
Invariably, this would select the word immediately after typing and entering
space, or other word breaking character (such as $, @, & etc for WebKit).

Once the subrange to replace has been determined, a new method in the editor
client (editorclient.h and editor_client_impl.cc), called
getAutoCorrectSuggestionForMisspelledWord, will be asked to provide the
autocorrect word. If it can provide one, then replace the subrange with that
word - otherwise, do nothing. Meanwhile,
getAutoCorrectSuggestionForMisspelledWord will eventually ask the SpellChecker
to provide it with the auto correct word, by using the WebViewDelegate from
EditorClientImpl. All the threading issues are automatically taken care of by
following the same path as that while determining whether a word is misspelled
in the Editor.

Note that if this feature is turned off from the UI, the
getAutoCorrectSuggestionForMisspelledWord will fast-return an empty string.

## Auto correct word determination algorithm in SpellChecker

The autocorrection algorithm resides in the SpellChecker Object in method
GetAutoCorrectWord. This method can contain a wide range of simple to
sophisticated algorithms for auto spell check.

As a starter, I propose to use the simplest algorithm - that of swapping
adjacent characters and checking to see if only *one* word is spelled correctly.
This is a very common case, as we tend to often type in one character before the
other while typing (at the very time of writing this sentence, I spelled "while"
as "hwile"!). As an example, consider the word "more". Swapping adjacent
characters would result in the following - "omre", "mroe", "moer", "more".
Obviously, only "more" is not misspelled. So, if the user types "omre", "mroe"
or "moer", it will be replaced with "more" as the auto suggest word.

As an anti-example, consider the (misspelled) word "noen". This will *not* be
auto corrected, because swapping adjacent characters will result in "neon", as
well as "none". Both are legitimate word - the user, thus, might have as well
meant either.

Once this basic algorithm is in place, I will devote my time over the next few
months to develop more sophisticated algorithms, with your help, to make this
feature a coveted one, unseen in other browsers so far.

## Chrome UI Support

For now, the only UI support required is the ability to turn this feature
on/off. This can be done as a check box below the "Allow spellcheck" checkbox in
Languages options menu. It can also appear in the sub context menu for
spellcheck options in a text box.

Obviously, more UI support will be eventually required. For example, users might
not want to have some words auto corrected - they can add those words from a
context menu command - such as perhaps "Stop autocorrecting this word". Also, a
UI element should appear below the autocorrected word when the cursor, or mouse,
if moved over it, to convey to the user that autocorrection has been done.

## Unit Testing

This infrastructure needs to have unit tests for the GetAutoCorrectWord method
in SpellChecker.
