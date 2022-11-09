---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: advancedspellchecker
title: Adaptive spell checking for multilingual users
---

## The existing spell checking system has all the features that most modern browsers have. It can suggest words, can add custom words, supports 27 languages (and counting), can install spellcheck dictionaries without user intervention, can change spell check languages on the 'fly', has 'new' words incorporated in many languages ([blog](http://blog.chromium.org/2009/02/spell-check-dictionary-improvements.html)), and is generally known to be stable and useful, without being intrusive. As a slightly more advanced feature, [automatic spelling correction](/developers/design-documents/automaticspellingcorrection) was added, as an experiment (under a command line flag). It has been moderately successful, albeit with some small nuances which need to be fixed before it can be fully incorporated as a feature.

At this stage, improvements to spell checking have to go beyond what every other
browser provides. One important aspect of spell checking is the fact that many
users are multi lingual. They would like to see spell check support which would
enable them to switch between their chosen languages of communication through
the browser smoothly, expecting the spell checking system to adapt to their
usage automatically, in real time. This would require the SpellChecker to
automatically detect the intended language of use as the user is typing, and
switch to it smoothly and non-intrusively.

Usage

This feature has to be designed such that the user involvement is minimum. The
user should be able to simply start typing in a text box (email, comment, doc
etc) and the spell checking system keeps pace to update itself to the language
the user is typing in. The user should be oblivious to the change of spellcheck
language in terms of performance while typing, and should be able to mix
multiple languages in a single text box to get the corresponding spell checking
done automatically. The suggestions to misspelled words should be presented
intuitively without cluttering up the context menu.

When the user mixes multiple languages together,the SpellChecker detects that
the user is using multiple languages, and starts supporting the languages
*simultaneously*. This means that, when the user types in a word, it is marked
misspelled if it is misspelled in all the languages. Only two concurrent
languages will be supported. If a new, third language is detected for use,
remove the one which was least recently used.

On right click on the misspelled word, the context menu shows sub context-menus
on the top, corresponding to the two languages, and the sub context-menus
themselves show the suggestions for the corresponding language.

Design principle

Thanks to prior work done to support this features, Chromium already includes a
rich set of features to enable this advanced feature. The following features
will be used:

Language detection

The Compact Language Detection (CLD) library was initially written for Google
Toolbar. It was ripped off from its dependency with toolbar, and was added to
Chromium recently. CLD is fully integrated with Chromium, and does not require
any communications with any server. It requires a very low sampling size - most
of the times it can detect the language from a single sentence. It is not very
resource intensive, and returns the language fast if the input is kept sparse.
It returns unknown for languages which it cannot detect.

SpellChecker support

The SpellChecker supports the ability to be changed on the fly. Currently, the
SpellChecker can handle one language at a time. It has to be upgraded to handle
multiple languages simultaneously, and should have the ability to quickly change
these languages on the fly. When this feature is enabled, given a word to check
for spelling and suggestions, it will do so for the languages it is supporting,
and will generate suggestions for these languages simultaneously.

UI Support

The UI has to be updated in several places to support this feature.

*Context menu suggestions*

The context menu over a text area currently shows suggestions in a single
language. It has to be updated so that it can now support sub context-menus to
show suggestions for different languages simultaneously.

*Sub context menu in Context menu for spell check options*

The context menu for a text area has a dedicated Spell Check options sub menu to
select language of the SpellChecker. Only one language can be selected through
context menu radio buttons at this time. When the auto detect language feature
is turned on, this has to now enable check boxes, which means that the languages
which have been detected so far will be shown, and two of them will be enabled
(by check boxes).

The spell checking system has to remember the languages typed by the user. Thus,
suppose the user types in English, French and German in a session. In a new
session, when the user starts using a text box, the SpellChecker will be set to
a default one, and these three languages will show up in the sub context menu,
with only the default one selected.

*Languages Options Menu*

There should be quick settings to turn this feature ON/OFF from the Language
Options menu.

*Note*: The Options menu for Languages is undergoing a change where the user can
simply set the languages used, and not bother explicitly about accept languages,
spell check languages and Chromium UI languages. The change in spellcheck
language will be reflected in the languages that the user uses. If the language
is not set by the user, it is added as a new language detected to be used by the
User.

**Limitations and possible drawbacks**

While this feature itself is exciting, past experience has revealed that such
features can be intrusive and counter intuitive for certain edge cases. Some
edge cases are the following:

*User mixes multiple languages in a single sentence:*

Sometimes, users, when constructing informal emails, mix words from the
different languages they know. Of course, the fact that multiple languages will
be supported simultaneously is the end solution - but how does one detect these
languages?

*How to decide the default language to start with*

Selecting the default language to start with during spell checking can be
tricky. Right now, the following heuristics will be used to select the default
language of the SpellChecker - the language of the web page if the text box is
empty - otherwise, the language of the words in the text box if it is
pre-populated.

Testing

The spell check tests have to be upgraded so that they are aware of the
SpellChecker's ability to spell in multiple languages simultaneously. Automated
tests have to be designed to check the ability of the SpellChecker to change
language on the fly adaptively with the language of the text being spell
checked..
