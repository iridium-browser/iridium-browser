---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: ui-strings
title: 10 steps to better user-facing strings
---

**1. Say it, then write it (aka, "Sound like a human")**

Say out loud, to a friend or to yourself, what you want to tell the user. Write
*that* down: it's going to be much more conversational than your first written
attempt.

*   Before you keep reading, go and actually write it down in a note or
            document.
*   As you read on, edit your string during each step.

**2. Focus on the user**

The user is the star! One way to literally do this when writing is to **set the
user as the subject** of the string ("You can do this action" instead of "Chrome
has released this feature").

*   **Lead with the goal:** It catches users' eyes and incentivizes them
            to keep reading. Prepositional intro phrases are useful: "*To
            perform your goal,* do this action."
*   **Offer a solution:** Instead of only describing what went wrong,
            explain how to fix it!
*   **Don't go behind the scenes:** Are you describing how a feature
            works? Take it out: users really, really don't care. \[*Exceptions:
            mandatory data transparency & privacy/legal text*\]

**3. Be consistent**

Are there any other pieces of related UI where we want to maintain consistency?
We have patterns for permissions prompts, settings, etc. Also, consider whether
there are others in our industry who do it well!

**4. Include everyone**

Use language that’s neutral to different cultures, races, genders, and age
groups. Avoid colloquialisms and US-centric references.

*   **Accessibility:** Does the string rely on color or position to
            identify a component? This won't work for people who are colorblind
            or use screen readers: find a **non-visual way to distinguish**
            components.

**5. Sound human**

User-facing text should be **useful, honest, and conversational**. Look for
words or phrases that sound formal or technical, and replace them with simple
words and phrases. [Plain language is for everyone, even
experts](https://www.nngroup.com/articles/plain-language-experts/), so this
applies as much as possible to developer-facing spaces as well.

Contractions are encouraged; for example: "Linux won't remember a USB device
after it's removed."

**6. Speak simply**

Ideally, UI text can be easily understood by a typical 11- or 12-year-old
student. Some tips:

*   Find the longest words in your sentence and swap them out for
            shorter synonyms. Example: "buy" instead of "purchase."
*   Look for conjunctions ("and," "or"): can you split one sentence into
            two sentences?
*   Count the words. Aim for 5-15 words per sentence.

**7. Be positive**

Are you telling a user what can't be done? Try flipping it around and telling
them what *can* be done, instead. You can describe limits with phrases like "up
to 25 MB" or "when it's available."

**8. Write globally**

Will the string translate well?

*   In some languages, it's hard to localize pronouns like "it" or
            "this." Avoid them when you can; or, at least, make sure they are as
            close as possible to the word they refer to.
*   If you can, avoid gerunds (verbs that act as nouns; end in "-ing").
*   Remove repetition: it's perceived as patronizing and a waste of
            time. For example: don't use the same string for both title & body;
            don't mention the product name multiple times.

**9. Keep it short (aka, "Short beats good")**

Does the string have to fit in a limited space? Keep in mind that English
strings may increase by at least 30% after translation (test with an [automated
translation](https://translate.google.com/) in Filipino, Greek, or German). Cut
everything that isn't *super critical*.

**10. Check the basics**

Here are some answers to common string questions:

*   Do we use the [serial
            comma](https://en.wikipedia.org/wiki/Serial_comma)?
    *   Yes!
*   Does the string need a period?
    *   Apple OSes:
        *   Fragments in titles/headings: no period
        *   Complete (single or multi) sentences: use periods
    *   Google OSes, Win & Linux:
        *   Single sentence: no period
        *   Multiple sentences: use periods
*   Which words do I capitalize?
    *   Apple OSes:
        *   Title Case = titles, buttons menus, menu items
        *   Sentence case = labels, instructional text
    *   Google OSes, Win & Linux:
        *   Sentence case = everything (except where strings overlap
                    with Apple OSes)
