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
page_name: media-gallery
title: MediaGallery
---

Proposal Date

*1/24/2012 updated 4/24/2012*

Who is the primary contact for this API?
*Steve VanDeBogart. This document was authored by Tony Payne*

Who will be responsible for this API? (Team please, not an individual)

*owp-media-gallery@*

Overview
Media and metadata access API.
Use cases
This API makes media applications much simpler and provides a standard interface
for both local and online galleries.
Do you know anyone else, internal or external, that is also interested in this
API?
This will be used in Q2 2012 by an internal application for both local
(removable media) galleries and remote galleries. At least 2 other internal
teams have expressed interest in using the API.

Could this API be part of the web platform?
We intend to make this part of the web platform but are looking to release an
experimental extension API first for 2 reasons: 1) to meet the aggressive goals
of the internal project, 2) To vet the API before going to standards bodies.
There are existing Gallery API proposals but they do not meet our needs (provide
no metadata access and rely on synchronous APIs)

Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
As mentioned above, we don't expect to go beyond the experimental stage with
this API. For backwards compatibility, the eventual OWP API can be wrapped by
the existing experimental API.

List every UI surface belonging to or potentially affected by your API:
*Settings: We may need a settings interface for adding and removing galleries
from the browser.*

*Permissions: As an extension, a manifest permissions entry will be used.
However, as a web platform API, applications that have not been given permission
to access the browser's galleries will cause a permissions prompt when they
attempt to access galleries for the first time.*

How could this API be abused?
*This API contains write and delete operations that can cause permanent data
loss.*

*Quite a lot of attention has been given to making it easy to write well-performing applications using this API but there is always the potential for trying to keep too much data in memory, since this API gives access to a potentially enormous amount of media file data on the system and online.*
Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) Delete all media files from the system
2) Corrupt all media files on the system
3) Upload all media files on the system to an evil host

4) Fill up the user's local drive

5) Copy malware onto the system, hoping the user will execute it.

Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?
*Yes. The MediaGallery API provides write and delete methods for making permanent changes to media files in the gallery.*
How would you implement your desired features if this API didn't exist?
*File access via existing HTML5 file APIs using a File dialog to gain permission
(this is a terrible user experience)*

*Metadata parsing using client-side parsers. These have terrible performance*

Draft API spec
*The current draft is in WebIDL format here:
<https://docs.google.com/a/google.com/document/pub?id=1UpwxoIBoTgrA9fLujXCirNp4NYB0Z0F0LiX_SuS9c5U>

*The previous draft can be found here:
<https://docs.google.com/a/google.com/document/pub?id=1RsLin39Zmun9kaxNUpChL55IoEHmLHF3EU8NRy28168>*

*We will update the draft style to match other extension APIs once we've
received more feedback.*

Open questions
*TBD*
