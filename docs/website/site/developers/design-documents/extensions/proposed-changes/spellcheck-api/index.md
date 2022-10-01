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
page_name: spellcheck-api
title: Spellcheck API
---

**Proposal Date**
**December 27, 2012**
**Who is the primary contact for this API?**
**rlp@**
**Who will be responsible for this API? (Team please, not an individual)**
**Spelling team**
**Overview**
**The proposed API would allow the user to load a new dictionary.**
**Use cases**
**Currently there are many languages which neither Chrome spellchecking nor our third party library Hunspell support (e.g., Zulu, Klingon, etc.). We routinely see bug reports requesting languages not yet supported (approximately 20-30% of Spellcheck bugs). There are avid users who have put together or would put together dictionaries if they knew they could use them. This API would provide an interface to allow them to write extensions to load the dictionary of their choice making it available for spellchecking.**
**Do you know anyone else, internal or external, that is also interested in this API?**
**We have several bugs that are for languages which we do not support and cannot find libraries. If we provided this API, those users could maintain their own dictionaries.**
**Could this API be part of the web platform?**
**It is unlikely that spellchecking will become part of the web platform.**
**Do you expect this API to be fairly stable? How might it be extended or changed in the future?**
**Yes. We will allow users to specify which format their dictionary is in. If at some point we change the format which we read dictionaries in, we will be able to update the API to keep the current format and simply convert to the new format.**
**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
**With this API we'd be adding the ability to have multiple dictionaries. If multiple extensions provided dictionaries, the only problem might be on loading if the user had an excessive number of dictionary-loading extensions as the loading was queued up.**
**List every UI surface belonging to or potentially affected by your API:**
**Settings-&gt;Language Settings: We would need to show the new dictionary as a language being used for spellcheck.**
**When spellchecking, red underlines and other spelling features would now be dependent on the dictionary available through this API.**
**Actions taken with extension APIs should be obviously attributable to an extension. Will users be able to tell when this new API is being used? How?**
**In the Settings-&gt;Language Settings dialog, we would show an icon or tag to indicate that a given dictionary came from an extension rather than a built in library.**
**How could this API be abused?**
**Dictionaries are just text files which are loaded in to Chrome. Possible abuse
could be adding words which one would normally expect to be spellchecked such
that they are not any more or adding "bad" words such as cursing or swear words.
Someone could also attempt to add a dictionary which is really code to be
executed upon loading.**

**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) Buffer overflow with a loaded dictionary.**
**2) Killing a normal spellcheck by creating a dictionary of common misspellings.**
**3) Creating a dictionary with code that executes upon loading.**
**What security UI or other mitigations do you propose to limit evilness made possible by this new API?**
**We would not allow binary dictionaries and we would sanitize all dictionaries before loading them, ensuring that they only consisted of words in the format we expect. This would eliminate 1 and 3. 2 is difficult to eliminate because if the user wants to add common misspellings as "correct" words, that's their choice and we would not want to prevent them from doing so.**
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**No. Once an extension using this API was removed, the dictionary it loads would no longer be loaded and the user would not see the dictionary in their Settings. Thus there would be no trace of this extension.**
**How would you implement your desired features if this API didn't exist?**
**We could provide a command line option for users to load a dictionary from a file on their system. The unfortunate aspect of this method is that users then have to find the file on the web and figure out how to pass it in from the command line which means only power users are likely to do so.**
**Draft Manifest Changes**
**Unknown -- see additional questions at bottom.**
**Draft API spec**
**Methods**
**loadDictionary**
**chrome.spellcheck.loadDictionary**

**Loads a dictionary based on the format specified by the user. This dictionary is automatically set to be used for spellchecking, but the user can stop using it for spellchecking via the Settings-&gt;Language Settings dialog at any time.**

**Parameters**
**dictionaryFile (string)**
**The path for the dictionary file. This may be either a url or a local file path.**
**dictionaryFormat(enumerated string \["hunspell", "text"\])**
**The user must indicate which format their dictionary is in: hunspell dictionary (concatentation of .aff and .dic files) or plain text.**
**result (int)**
**The result is a int indicating success or an error code in loading the dictionary.**
**Example:**
**function loadMyDictionary(myDictionaryFile) {**
**var result = chrome.experimental.spellcheck.loadDictionary(myDictionaryFile);**
**if (!result) {**
**// Indicate to user that dictionary load failed through dialog/infobar/etc.**
**}**
**}**
**Open questions**
**Is a dictionaryLoaded event necessary?**
**Do we need an optional permission to load the dictionary?**
