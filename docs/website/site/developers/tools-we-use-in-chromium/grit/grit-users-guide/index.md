---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/tools-we-use-in-chromium
  - Tools we use in Chromium
- - /developers/tools-we-use-in-chromium/grit
  - GRIT
page_name: grit-users-guide
title: GRIT User's Guide
---

[TOC]

# Getting Started

Your chromium checkout contains GRIT. Run ./tools/grit/grit.py for help. GRIT
integrates easily with [GYP](http://code.google.com/p/gyp/), see
[examples](https://code.google.com/p/chromium/codesearch#search/&q=grit%20file:%5C.gyp&sq=package:chromium&type=cs).

It's a good idea to run GRIT's unit tests to check that your installation is OK.
Run grit unit, which should print the result of the tests, ending with OK.

## Creating a Blank .grd File

You can run grit newgrd to create a new .grd file with no content but a skeleton
to start you off with.

If you're looking to convert an existing .rc file to the .grd format, see the
section "Converting Your RC File to GRIT Format" below.

# Adding Resources

## Includes (e.g. GIF, BMP, ICO etc.)

Here are the steps to add an included resource, e.g. a GIF or BMP. Note that
this does not include HTML templates, those are considered "structures" (see
section below).

1.  Open your project's .grd file.
2.  Find the latest &lt;release&gt; element (the one with the highest
            number in the 'seq' attribute). These elements group together
            resources that are part of the same public release. Note that some
            projects always use just a single &lt;release&gt; element.
3.  Under that &lt;release&gt; element, find an &lt;includes&gt;
            element.
4.  Add an &lt;include&gt; element to the end of that &lt;includes&gt;
            element. The &lt;include&gt; element has the following attributes,
            and no body:
    *   type: This goes into the RC file verbatim, so it is the resource
                type string you want to use for this include, e.g. 'GIF'
    *   name: This is the textual ID of this resource. GRIT will
                generate a #define in the resource header file for this textual
                ID, so you can use it in your code as long as you include the
                resource header file.
    *   file: This is the path to the file to include, **relative to the
                .grd file**

## i18n Messages

***Note: readers are here advised to also read [UI Localization (aka
Translations)](/developers/design-documents/ui-localization).***

Similarly for includes (above), find the latest &lt;release&gt; element, then
find a &lt;messages&gt; node under that. Add an i18n &lt;message&gt; node under
it. i18n &lt;message&gt; nodes have the following attributes:

*   *name*: The textual ID of the message.
*   *desc*: A description of the message giving enough context to the
            translator to translate the message correctly (e.g. the message
            "Shut" might be a description of an action you need to take or the
            description of the status of something, so a description like e.g.
            "Shut the current dialog; button label" would help translators do
            the right thing).
*   *meaning*: You can use this field to ensure that two messages that
            have the same text will not necessarily share the same translation.
            This can provide a bit of context to the translators along with the
            'desc' attribute.
*   *internal_comment*: If you want to add a comment for use internally
            (by the developers) this is the place to put it.

The body of the &lt;message&gt; element is the text of the message. Inline in
the text, you should use &lt;ph&gt; elements to indicate *non-translatable*
sections (human or automatic translators will leave the &lt;ph&gt; element
content alone - they will not try to translate it).

The &lt;ph&gt; element has a single attribute, '*name*', which you use to give
the placeholder a name (which must be uppercase and should usually be
descriptive, e.g. USER_NAME or TIME_REMAINING). Apart from the
*non-translatable* text, the &lt;ph&gt; element can contain a single&lt;ex&gt;
element containing an example of what the placeholder could be replaced with.
This is shown to the translators, and could be e.g. "Jói" for a placeholder with
a name of USER_NAME.

For example:

&lt;message name="ID_OF_MESSAGE"&gt;
Hello &lt;ph name="USER_NAME"&gt;%s&lt;ex&gt;Joi&lt;/ex&gt;&lt;/ph&gt;, how are
you today?
&lt;/message&gt;

GRIT allows you to use messages that begin or end with whitespace. You need to
put ''' (that's three single quotes, like Python's triple quotes) before leading
whitespace and after trailing whitespace, because the default is to strip
leading and trailing whitespace from messages.

### Example

Given a message that looked like the following in the RC file:

IDS_EMAIL_ESTIMATE "%d of about %d emails - "

This should be represented as the following in the .grd file:

&lt;message name="IDS_EMAIL_ESTIMATE"
internal_comment="TODO make the placeholders reorderable so the translators can
rearrange things"
desc="Shown at top of results page"&gt;
&lt;ph name="ITEM"&gt;%d&lt;ex&gt;1&lt;/ex&gt;&lt;/ph&gt; of about &lt;ph
name="ESTIMATED_TOTAL"&gt;%d&lt;ex&gt;130&lt;/ex&gt;&lt;/ph&gt; emails - '''
&lt;/message&gt;

Let's walk through what's going on here:

*   The '*name*' attribute of the message element is the symbolic ID of
            the message, that you use to refer to the message in e.g. your C++
            code. GRIT generates a numeric ID for the message which goes into
            the resource.h file
*   The '*desc*' attribute is a description of the message which should
            give the translators enough context to translate it properly. This
            is especially important for short messages such as a single word,
            where you have to at least disambiguate whether the word is a noun,
            verb, adjective, and whether it's used in a singular or plural
            context.
*   The '*internal_comment*' attribute is for any comments related to
            this message that the translator should not see (e.g. TODO comments,
            comments on how to use in code, etc.)
*   We change the two %d format specifiers into "placeholders," which
            are represented by the &lt;ph&gt; element. Everything that should
            not be translated by the translators needs to be changed into a
            placeholder.
*   The '*name*' attribute of the placeholder must be composed of the
            capital letters A-Z, numbers 0-9 and the underscore character. It
            should describe the placeholder succinctly. The placeholder names
            are used to provide the translator with translatable text, in this
            case "ITEM of about ESTIMATED_TOTAL emails - ".
*   The contents of the &lt;ph&gt; element should be what you want your
            program to see, i.e. what goes into the compiled RC. Basically this
            is the original text.
*   You can (and should) add a single &lt;ex&gt; node as a child of the
            &lt;ph&gt; node. The contents of the &lt;ex&gt; node will be used to
            create an example of a "final" message to help the translators. This
            shows what the message might look like once all format specifiers
            have been filled in, in this case "1 of about 130 emails - "
*   Because the message ends in a space character, and you don't want to
            lose that space character, you need to add a triple quote, ''',
            after the end of the message. This prevents GRIT from stripping the
            trailing whitespace. The triple quote does not become part of the
            message.

## Structures (menus, dialogs, HTML templates, etc.)

Any resource that is not a simple message and contains one or more translateable
parts needs to be defined as a &lt;structure&gt; element. Similar to includes
and messages (see above), you find the latest &lt;release&gt; element, find a
&lt;structures&gt; node under that, and add a&lt;structure&gt; node to it.
&lt;structure&gt; nodes have the following attributes:

*   *name*: The textual ID of the structure. For menus, dialogs and
            VERSIONINFO resources, this must match the ID used in the .rc file
            the structure is kept in.
*   *type*: The type of structure. This can be 'menu', 'dialog',
            'version', 'rcdata', 'tr_html', 'txt' or 'admin_template'. The first
            four indicate a MENU, DIALOG (or DIALOGEX), VERSIONINFO or RCDATA
            resource in a .rc file. 'tr_html' indicates an HTML template,
            'admin_template' indicates an .adm file (used for enterprise
            features on Windows) and 'txt' indicates a plain text file that
            needs to be translated for different languages.
*   *file*: The file, **relative to the .grd file** that the structure
            is kept in. If it is a structure type taken from an .rc file, the
            structure resides in the file along with possibly lots of other
            structures, and is looked up by its textual ID (the 'name'
            attribute). Otherwise it takes up the whole file (i.e. the whole
            contents of the file are the contents of the &lt;structure&gt;
            element).

Note that GRIT only has support for symbolic resource IDs (IDs that are
preprocessor defines that are given a numeric value) and not for string literal
resource IDs. The latter are suboptimal, since they increase the size of the
resources. If the .rc file you point to uses string literal resource IDs, you
will have to #define those string literals manually to use those resources. The
recommended approach is to only use symbolic resource IDs.

## Splitting Resources into Multiple Files

It is possible to have 'sub' grd files, the ones with .grdp extension.

They can be include in .grd files using the &lt;part&gt; element. See [this
example](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/app/bookmarks_strings.grdp&sq=package:chromium&type=cs).

# Outputs and Translations

By calling grit build you make GRIT build RC files and resource header files
from a .grd file. This section details how to specify which files to output, how
to specify the translation files to use, and how to use conditional output of
resources.

## Specifying Output Files and Translation Files

The way you specify which files grit build should output is using an
&lt;outputs&gt; section in the .grd file. NOTE: The &lt;outputs&gt; section
should be put at the top of the .grd file, as this can speed up GRIT when used
with build systems that scan the .grd file for dependencies.

To specify the translation files to use, you use a &lt;translations&gt; section
that points to each of the .xtb (translation bundle) files containing the
translations.

The top of your .grd file, including the &lt;outputs&gt; and
&lt;translations&gt; sections might look like this:

&lt;?xml version="1.0" encoding="UTF-8"?&gt;
&lt;grit latest_public_release="0" source_lang_id="en" current_release="1"
base_dir="../.." enc_check="möl"&gt;
&lt;outputs&gt;
&lt;output filename="resource.h" type="rc_header" /&gt;
&lt;output filename="en/generated_resources_en.rc" type="rc_all" lang="en" /&gt;
&lt;output filename="fr/generated_resources_fr.rc" type="rc_all" lang="fr" /&gt;
&lt;output filename="ja/generated_resources_ja.rc" type="rc_all" lang="ja" /&gt;
&lt;/outputs&gt;
&lt;translations&gt;
&lt;file path="$TRANSLATIONS_FOLDER/fr.xtb" lang="fr" /&gt;
&lt;file path="$TRANSLATIONS_FOLDER/ja.xtb" lang="ja" /&gt;
&lt;/translations&gt;
&lt;!-- Remainder of .grd file --&gt;
&lt;/grit&gt;

Something to note here: The language code is included in the filename of each of
the generated .rc files, even though it is already a component of the files'
paths. This is recommended to ensure that the .res intermediary files which are
generated by the RC compiler from the .rc files and then linked into the
executable do not share the same filename (as they may all be built into one
directory even if the GRIT output files are in separate directories).

## Conditional Output of Resources

In an .rc file, it's common to use C preprocessor commands to control which
parts of the file get output given the existence or values of C preprocessor
defines. GRIT has this same capability, in addition to allowing output of
resources to be conditional on which language is being output.

To use conditional output of resources, simply enclose some of your
&lt;message&gt;, &lt;structure&gt; or &lt;include&gt; elements in an &lt;if&gt;
element. The 'if' element has a single attribute 'expr', the contents of which
are evaluated as a Python expression, and the resources within the 'if' element
are only output if the result of that expression is true. Your Python expression
has access to the following objects and functions:

*   lang is the language code ([IETF language
            tag](http://en.wikipedia.org/wiki/IETF_language_tag)) being output,
            taken directly from the 'lang' attribute of the &lt;output&gt; node
            currently controlling the output.
*   defs is a map of C preprocessor defines to their values, e.g. {
            'SHIPPING' : True, '_DEBUG' : False }. Note that preprocessor
            defines with values 1 and 0 get changed so their values are True and
            False, respectively.
*   pp_ifdef(name) behaves just like an #ifdef statement in the C
            preprocessor, i.e. it returns True if name is defined (exists as a
            key in defs).
*   pp_if(name) behaves just like an #if statement in the C
            preprocessor, i.e. it returns True if and only if name is defined
            and its value is true.

Note that conditional output of resources only affects those output files of
GRIT that determine which resources get linked into your executable, i.e.
currently only the .rc files output. It has no effect on resource header files,
or translation interchange files like .xmb files. This is desirable for several
reasons, e.g. resource IDs stay the same regardless of conditional output, all
messages always go to translation, etc.

## Avoid unnecessary header includes

GRIT by default includes some header files (for example: atlres.h). In order to
avoid these includes you can use the &lt;emit&gt; tag as a child
of&lt;output&gt;. Doing this would replace all the includes with your customized
includes. Including an empty &lt;emit&gt; tag makes GRIT include no header files
in the generated resource file.

&lt;output filename="resource.grh" type="rc_header"&gt;
&lt;emit&gt;#include "foo/bar.h"&lt;/emit&gt;
&lt;/output&gt;

# GRIT Cookbook

This section documents some common "recipes" for getting things done using GRIT.

## Getting Ready for Release

When you are about to release, you will probably want to make sure none of your
messages get automatically pseudotranslated, i.e. you want to have 'grit build'
fail if there are some missing translations. To achieve this goal, you simply
add a allow_pseudo="false" attribute to your&lt;release&gt; node (this attribute
is true by default), e.g.

&lt;release seq="1" allow_pseudo="false"&gt;
&lt;!--
If GRIT finds a missing translation for any message or any part
of any structure within this element, it will fail rather than
create an automatic pseudotranslation.
--&gt;
&lt;messages&gt;
...
&lt;/messages&gt;
&lt;structures&gt;
...
&lt;/structures&gt;
&lt;/release&gt;

When you start working on your next release, **leave everything inside that
&lt;release&gt; element alone** (or at least don't modify or add to it - OK to
remove from it). Instead, create a new &lt;release&gt; element and put the
resources you add to your new release inside that element. If you need to modify
one of the existing resources, move it from the old release to the new release,
and modify it within the new release.

## Language-Specific Graphics (or Other Included Resources)

There is often a need to localize graphics, e.g. logos that have text on them.
It's easy to tell GRIT to use different logos for different languages:

&lt;!-- Other parts of .grd file snipped --&gt;
&lt;includes first_id="2048"&gt;
&lt;if expr="lang in \['de', 'en', 'es', 'fr', 'it'\]"&gt;
&lt;include type="GIF" name="IDR_LOGO" file="logo.gif" /&gt;
&lt;/if&gt;
&lt;if expr="lang == 'ja'"&gt;
&lt;include type="GIF" name="IDR_LOGO" file="ja_logo.gif" /&gt;
&lt;/if&gt;
&lt;if expr="lang == 'zh_cn'"&gt;
&lt;include type="GIF" name="IDR_LOGO" file="zh_cn_logo.gif" /&gt;
&lt;/if&gt;
&lt;/includes&gt;
&lt;!-- Other parts of .grd file snipped --&gt;

Here we use the same logo for English and FIGS languages, and different logos
for Japanese and Simplified Chinese.

## Language-Specific Layout of Dialog Boxes

It's common for dialog boxes to require a different layout for languages other
than English, as most languages are much more verbose than English (30-40% on
average). Of course the best way to handle this is to make the dialog boxes for
English roomy enough to account for all languages, but this is not always
possible due to aesthetics or project constraints. Likewise, other structured
resources, such as HTML templates, sometimes need to have locale-specific
versions.

Although you *could* use a few &lt;if&gt; nodes to achieve something close to
what you want, this would mean each of your different structured resource
variants would need to be in English. It would be much more convenient to be
able to have your variants in the target language (so that you can easily size
e.g. the dialog correctly) yet pick up the source messages from the original
structured resource. This is what the &lt;skeleton&gt;node is for.

Here's how you would use it:

&lt;!-- Other parts of .grd file snipped --&gt;
&lt;structures first_id="4096"&gt;
&lt;!-- The file specified in the &lt;structure&gt; node is used except when one
of the expressions
in &lt;skeleton&gt; nodes is true. --&gt;
&lt;structure type="dialog" name="IDD_DIALOG" file="structured_resources.rc"&gt;
&lt;!-- The variant_of_revision attribute is mandatory and intended to specify
which source
control revision number this skeleton variant is a variant of. Useful so that
GRIT can
automatically detect if you may need to update your skeleton (not implemented
yet). --&gt;
&lt;skeleton expr="lang == 'ja'" file="ja\\IDD_DIALOG\\IDD_DIALOG.rc"
variant_of_revision="3" /&gt;
&lt;/structure&gt;
&lt;/structures&gt;
&lt;!-- Other parts of .grd file snipped --&gt;

To help you work with variant skeletons, there is a tool grit resize that will
output a Microsoft Visual Studio project file and an .rc file containing the
dialogs you request in the language you request. See grit help resize for more
details.

NOTE: The &lt;skeleton&gt; node currently only works for structures with
*type="dialog"*. Support for it can be added to other structure types but
usually what you want for e.g. HTML structures can usually be accomplished more
easily by using separate files and selecting between them with &lt;if&gt; nodes
(see the above section).

## Language-Specific Messages

Occasionally, there is a need for a message to have a different meaning in one
language than it does in another. An example of this is that a Terms of Use
document might have to have different clauses for the U.S. than it does for
Germany. It's easy to give GRIT different variants of the same message to use
for different languages:

&lt;!-- Other parts of .grd file snipped --&gt;
&lt;messages first_id="10240"&gt;
&lt;if expr="lang != 'de'"&gt;
&lt;message name="IDM_TERMS_OF_USE" desc="Do not translate this message into
German"&gt;
Use this product at your own risk.
&lt;/message&gt;
&lt;/if&gt;
&lt;if expr="lang == 'de'"&gt;
&lt;message name="IDM_TERMS_OF_USE" desc="This message only needs to be
translated into German"&gt;
Use this product at your own risk, except as limited by German law.
&lt;/message&gt;
&lt;/if&gt;
&lt;/messages&gt;
&lt;!-- Other parts of .grd file snipped --&gt;

There are a couple of things to note here:

*   It's a good idea to use the 'desc' field (as done in the example
            above) to let the translators know if a message only needs to be
            translated into a particular language, or into all languages except
            some particular language(s).
*   You are providing a different **English** message to use for a
            particular language (German in this case) but what actually gets
            output to the German RC file is the German translation of that
            English message.

## Getting Translations of Strings in Images

You may have a bunch of images that contain some text, and wouldn't it be nice
to be able to get that text translated along with everything else? You can do
this by following this recipe:

&lt;!-- ... --&gt;
&lt;includes first_id="1024"&gt;
&lt;!-- ... --&gt;
&lt;include type="GIF" name="IDR_SUBMIT_BUTTON" file="submit_btn.gif" /&gt;
&lt;/includes&gt;
&lt;!-- ... --&gt;
&lt;messages&gt;
&lt;!-- ... --&gt;
&lt;if expr="False"&gt;
&lt;message name="IDM_MESSAGE_FOR_PICTURE_IDR_SUBMIT_BUTTON"&gt;
Submit
&lt;/message&gt;
&lt;/if&gt;
&lt;/messages&gt;
&lt;!-- ... --&gt;

The trick here is to match the message with the include by using a simple naming
convention (the message node's 'name' is the include node's 'name' with
"IDM_MESSAGE_FOR_PICTURE*" prefixed), and to prevent the message from getting
included in your executable by putting it into an &lt;if expr="False"&gt; block
(keep in mind that &lt;if&gt; blocks only affect output of resources to .rc
files, and not to .xmb files or other translation interchange formats GRIT may
support in the future).*

After the translators have finished their job, you'll be able to find your
image's translated text in the .xtb file for the language(s) in question.

## Translate by Example

In some cases, we have resources that difficult for a non-programmer to
translate. An example is a date picture string, which might look like "M'/'d
YYYY". A useful thing to do here is to get the translator to translate an
example of what the result of applying that format string might look like. Then
as a programmer you can take the translated example and translate it back into a
picture string. The recipe for this would be:

&lt;!-- ... --&gt;
&lt;messages first_id="1024"&gt;
&lt;!-- ... --&gt;
&lt;if expr="False"&gt;
&lt;message name="IDM_DATE_FORMAT_EXAMPLE_TO_TRANSLATE"&gt;
6/6 2005
&lt;/message&gt;
&lt;/if&gt;
&lt;message translateable="false" name="IDM_DATE_FORMAT"&gt;
M'/'d YYYY
&lt;/message&gt;
&lt;/messages&gt;
&lt;!-- ... --&gt;

Here we add a message IDM_DATE_FORMAT_EXAMPLE_TO_TRANSLATE that gets sent for
translation, but not added to your executable (because it's enclosed in an
if-false block). The real resource is marked non-translatable, so it doesn't get
sent for translation. Once the translators are done, you look for the message in
the .xtb file, interpret the examples back into picture strings, and add them to
your .grd file using conditional output to control which one gets used for which
language:

&lt;!-- ... --&gt;
&lt;messages first_id="1024"&gt;
&lt;!-- ... --&gt;
&lt;if expr="False"&gt;
&lt;message name="IDM_DATE_FORMAT_EXAMPLE_TO_TRANSLATE"&gt;
9/23 2005
&lt;/message&gt;
&lt;/if&gt;
&lt;if expr="lang == 'en'"&gt;
&lt;message translateable="false" name="IDM_DATE_FORMAT"&gt;
M'/'d YYYY
&lt;/message&gt;
&lt;/if&gt;
&lt;if expr="lang == 'fr'"&gt;
&lt;message translateable="false" name="IDM_DATE_FORMAT"
internal_comment="Translation of example was '23/09 2005'"&gt;
dd'/'MM YYYY
&lt;/message&gt;
&lt;/if&gt;
&lt;/messages&gt;
&lt;!-- ... --&gt;

# Miscellaneous GRIT extensions and tips

GRIT has several other features you can use to help with localization.

## Using expandable variable to localize the link in html files

In html files, it is common that the ref link needs to point to the localized
page, for example,

http://www.example.com/support/myproduct?lang={language}

In order for the link to point to the correct language, when GRIT is generating
the localized html files from the html template, we set the langparameter to an
expandable GRIT variable
GRITLANGCODE[?](https://code.google.com/p/grit-i18n/w/edit/GRITLANGCODE), so
that it will be expanded to the correct langauge code.

In a struct node, enable the HTML template to use expandable variable by setting
expand_variables attribute to "true". Here is an example.

&lt;structure name="AFINTRO.HTML" encoding="utf-8" file="afintro.html"
type="tr_html" generateid="false" expand_variables="true"/&gt;

In the html template, set the lang parameter of the link to
GRITLANGCODE[?](https://code.google.com/p/grit-i18n/w/edit/GRITLANGCODE). Here
is an example.

&lt;a href="http://www.example.com/support/myproduct?lang=\[GRITLANGCODE\]"
target="_blank"&gt;Go to site&lt;/a&gt;

## Using expandable variable to generate HTML files for RTL languages

To enable GRIT to generate right-to-left (RTL) layout HTML files for RTL
languages, we can use the special expandable variable
GRITDIR[?](https://code.google.com/p/grit-i18n/w/edit/GRITDIR).

In struct node, enable the HTML template to use expandable variables by setting
the expand_variables attribute to "true".

&lt;structure name="AFINTRO.HTML" encoding="utf-8" file="afintro.html"
type="tr_html" generateid="false" expand_variables="true"/&gt;

In the HTML template, embed
GRITDIR[?](https://code.google.com/p/grit-i18n/w/edit/GRITDIR) variable in the
&lt;html&gt; tag, here is an example.

&lt;html style="width:29em; height:24em"
XMLNS:t="urn:schemas-microsoft-com:time" \[GRITDIR\]&gt;

## Using expandable variable to set the language attribute in product version information correctly in VS_VERSION_INFO of rc file

In order to have Language attribute of product version information show the
correct language information for client application, the VS_VERSION_INFO section
has to set the attributes using correct LCID and charset code. Again, we can use
grit expandable variables to make life easier. Here is an example.

VS_VERSION_INFO VERSIONINFO
...
BEGIN
BLOCK "StringFileInfo"
BEGIN
BLOCK "\[GRITVERLANGCHARSETHEX\]"
BEGIN
...
END
END
BLOCK "VarFileInfo"
BEGIN
VALUE "Translation", \[GRITVERLANGID\], \[GRITVERCHARSETID\]
END
END

## Running processing after translation of structures

Sometimes, it may be necessary to run additional processing of a translated file
(structure) after translation has occurred. To do this, use therun_command
attribute on the structure element. To add the filename to the command, use
%(filename)s. For instance, to run type on a file after it's been processed by
grit, do:

&lt;structure name="AFINTRO.HTML" encoding="utf-8" file="afintro.html"
type="tr_html" run_command="type %(filename)s"/&gt;

# Migrating Your Project

## Converting Your RC File to GRIT Format

The grit rc2grd tool takes an existing RC file, which should be the RC file for
the source language, and converts it into a .grd file.

Because of the way grit rc2grd works, a bit of preparation of the original RC
file is required.

*   grit rc2grd takes any existing comments on your string table
            messages and uses them as the description field for the message.
    *   The description field for a message is output as a 'description'
                attribute to the .xmb file for your project, so that it can be
                shown to translators when they are translating the message.
                There is a separate field in .grd files, internal_comment, that
                can be used for comments about messages that are intended only
                for programmers, not for translators.
    *   Comments belonging to a message are identified as all comments
                above that message after the last message parsed. This means if
                you use a commenting style where comments are immediately behind
                the message, you need to move them up above the message.
    *   In your .rc file, you might have used a single comment for a
                whole block of messages. If you want this comment to be seen by
                translators when they edit any of these messages, you need to
                duplicate it to each of the messages instead of having it only
                at the start of the block.

When you open up the .grd file, you will notice that some of your stringtable
messages have been changed so that HTML code and printf orFormatMessage format
specifiers have been replaced with placeholders (&lt;ph&gt; nodes). For all
placeholders that were put in place of printf andFormatMessage format
specifiers, the placeholder name starts as TODO_XXXX and the example is set to
TODO. This allows you to go through the generated .grd file and replace the
TODO_XXXX placeholder names with sensible names (ones that will help the
translators) and the example text with an actual example of what might be put in
place of this placeholder (for instance if the placeholder name is NUMBER then
the example might just be "5").

The .grd file that gets generated will refer to your original .rc file for any
menus, dialogs and version information resources. You can continue using e.g.
the VisualStudio[?](https://code.google.com/p/grit-i18n/w/edit/VisualStudio)
resource editor to edit these resources in the original .rc file. However, once
you've started building your project using the .rc file generated from the .grd
file by grit build, you should delete stringtables and "includes" from your
original .rc file as these should now be in the .grd file and you don't want
people to get confused and edit them in the old location.

Note that the grit rc2grd tool only supports symbolic resource IDs and will not
transfer resources that are identified by string literals over to the .grd file.
It is recommended not to use string literals as resource IDs.
