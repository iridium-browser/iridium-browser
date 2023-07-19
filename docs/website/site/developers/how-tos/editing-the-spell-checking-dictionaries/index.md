---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: editing-the-spell-checking-dictionaries
title: Editing the spell checking dictionaries
---

Each hunspell dictionary comes in two files. The .dic file which is the list of
words, and the .aff file which is a list of rules and other options. When adding
new words to existing dictionaries, you should only add to the .dic_delta file,
but you may need to refer to the .aff file for several things. The .dic files
are overwritten when dictionaries are updated, so they should not be modified.

We maintain some additions to the dictionary files which we keep in a separate
file ending in .dic_delta. Keeping the original unchanged allows us to more
easily pull updated versions from the [dictionary
maintainers](http://lingucomponent.openoffice.org/). Chromium actually reads an
optimized format which uses the suffix .bdic. This file is generated from the
.aff, .dic, and .dic_delta files using the convert_dict tool in the Chromium
project. All of these files are checked into
[deps/third_party/hunspell_dictionaries/](http://src.chromium.org/viewvc/chrome/trunk/deps/third_party/hunspell_dictionaries/)
in Subversion, and checked out into src/third_party/hunspell_dictionaries when
syncing locally.

#### Encoding issues

The .dic and .aff files are normally in the encoding most appropriate for that
language, which is unfortunately not usually UTF-8. You will need to open the
files in an editor with that encoding enabled. Most of the Western European
languages use Latin1 which should be easy. For other ones, search in the .aff
file for the line that begins with "SET" to see which character set it uses.

The .dic_delta files are always in UTF-8! This is confusing, but it allows us to
additional flexibility to add foreign words. The .bdic files are always UTF-8
internally, and the convert_dict tool converts things appropriately when it
runs.

## To help

Our .dic_delta files do not generally have affix rules. These rules tell
Hunspell, for example, how to convert a word into its plural or possessive
forms. The rest of this document explains how to do this.

The easiest way to get started is to find a word in the .dic file that is like
the word you're looking at in the .dic_delta file, and copy the rules. For
example, en_US.dic contains the entry cat/SM. If you want to add the word
"raccoon" and all of its variants to the dictionary, then add raccoon/SM to
en_US.dic_delta.

If the .dic files contains incorrect rules for a word, then add the word with
the correct rules to .dic_delta. For example, if en_US.dic contains the entry
raccoon/M, but the rule should be /MS, then add raccoon/MS to en_US.dic_delta.
Rules for entries in .dic_delta override the rules for entries in .dic.

You can also help by reviewing word lists of words we've identified as popular
but not in the dictionary. Please contact brettw AT chromium.org if you are
interested.

## Full details

Affix rules optionally follow each word in the dictionary file. These are
separated from the word using a slash. Each file uses one of two formats. It
will generally be obvious which one is which. Affix rules tell you what prefixes
and suffixes (affixes) can apply to the word. I will describe as much as you
need to know. There are additional details at
<http://sourceforge.net/docman/display_doc.php?docid=29374&group_id=143754>

### Explicit affix rules

Explicit affix rules look like a string of letters and numbers after each word.
This is the most common type. For example, here are some lines from the French
file:

> aberrante/LMF
> aberration/LMS
> aberre/jnu
> aberré
> aberrer/nM

Each one of the letters indicates a rule that can possibly apply. You can see
that aberré has no rules. This is OK. The other words have a sequence of rules:
each rule is identified by a unique letter. Case matters so "F" is different
than "f".

Each rule is in the .aff file for that language. The rules come in two flavors:
SFX for suffixes, and PFX for prefixes. Each line begins with PFX/SFX and then
the rule letter identifier (the ones that follow the word in the dictionary
file:

> PFX &lt;*rule_letter_identifier*&gt; &lt;*combineable_flag*&gt;
> &lt;*number_of_rule_lines_that_follow*&gt;

You can normally ignore the combinable flag, it is Y or N depending on whether
it can be combined with other rules. Then there are some number of lines
(indicated by the &lt;*number_of_rule_lines_that_follow*&gt;) that list
different possibilities for how this rule applies in different situations. It
looks like this:

> PFX &lt;*rule_letter_identifier*&gt; &lt;*number_of_letters_to_delete*&gt;
> &lt;*what_to_add*&gt; &lt;*when_to_add_it*&gt;

For example:

> SFX B Y 3
> SFX B 0 able \[^aeiou\]
> SFX B 0 able ee
> SFX B e able \[^aeiou\]e

If "B" is one of the letters following a word, then this is one of the rules
that can apply. There are three possibilities that can happen (because there are
three lines). Only one will apply:

*   *able* is added to the end when the end of the word is "not"
            (indicated by "^") one of the letters in the set (indicated by "\[
            \]") of letters a, e, i, o, and u. For example, *question* →
            *questionable*
*   *able* is added to the end when the end of the word is "ee". For
            example, *agree* → *agreeable.*
*   *able* is added to the end when the end of the word is not a vowel
            ("\[^aeiou\]") followed by an "e". The letter "e" is stripped (the
            column before *able*). For example, *excite* → *excitable.*

PFX rules are the same, but apply at the beginning of the word instead for
prefixes.

### Numbered affix rules

Some dictionaries use numbers instead of a list of affix rules. Each number is a
unique combination of rules, which is convenient since there are usually not
very many unique combinations of rules that can apply. This just means an extra
step. For example, the English dictionary has:

> Abeu/6
> abeyance/7
> abeyant
> Abey/6

You can see that abeyant has no rules, so it is never conjugated. The other
rules are 6 and 7. To see what these are, look up in the .aff file. The "AF"
lines you which set of rules correspond to each number. For example, the
en-US.aff file has a bunch of lines like this:

> AF mn # 1
> AF 1n # 2
> AF pt # 3
> AF p # 4
> AF ct # 5
> AF M # 6
> AF MS # 7
> AF DGLRS # 8

You can then see that 6 corresponds to the rule sequence "M" and 7 corresponds
to the rule sequence "MS". You would look these sequences of rules up just like
the explicit affix rules discussed above. So you look up in the PFX and SFX
lines and find the rule for "M" is:

> SFX M Y 1
> SFX M 0 's .

Which means to make it possessve. For example, *Abey* → *Abbey's.*
