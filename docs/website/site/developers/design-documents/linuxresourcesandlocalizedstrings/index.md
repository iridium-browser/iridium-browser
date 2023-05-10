---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: linuxresourcesandlocalizedstrings
title: LinuxResourcesAndLocalizedStrings
---

Resources (and localized strings) on Linux
**Goals**
Find a way to load binary data blobs and localized strings on linux. It's not an
explicit goal for us to provide a solution for OSX, but a cross platform
solution would be preferred.
**Background**
On Windows, we store resources in dll files. They are compiled into dll files
using Visual Studio's resource compiler (rc.exe) which read .rc files. .rc files
are just lists of resource ids and the actual file on disk that they represent
or an inlined string (for translations). From code we reference the resource
using the resource ids which are found in a header file. For localized strings,
we use GRIT to generate the .rc files.
Most resources are compiled directly into chrome.dll. There are two exceptions:
theme images are in theme.dll and languages are in locale specific dlls (e.g.,
en-US.dll). Having a separate theme.dll was to make it easier to transition to a
real theming system and to make it easier for UI developers to test changes to
the theme (by not having to re-link chrome.dll). Locale data is in a separate
dll because we would like to reduce the chrome download size by providing
localized builds of chrome. One would only need to download the strings for
languages that are of interest. This doesn't save much in download right now,
but if ICU data files could be split into chunks, then it could save &gt; 1mb of
download.
On Linux, data resources are normally just kept as files on disk that are read
during runtime. The location of these resources are normally hard-coded into the
binary at compile time. Localized strings are normally handled using gettext.
From code, gettext uses the English string to reference the localized string.
gettext has another interface called catgets which can be used to reference
strings using a resource id. This is pretty similar to what we do on Windows.
**Requirements**
The standard Linux method of reading data resources from disk seems like a
non-starter because we use lots of small image files to create the Chromium UI.
We could possibly work around this by image spriting, but we would still read
more files from disk that windows currently is. Also, while catgets might be a
good solution for localized strings, it doesn't handle binary data.
**Design Ideas**
Roll our own resource handling. This would involve a disk format for data
resources and strings, and code to load the file and read ranges of bytes from
the file. To create the disk format, we would use a python script that reads the
.rc files at compile time and generates our data file. This would be similar to
the Visual Studio resource compiler except it wouldn't be able to put data
directly into the dll (shared object). The code to load the file and read a
range of bytes would exist in base so we can load data resources (like the tld
data file) in sub module tests.
The main drawback is that we won't be able to bundle binary data blobs directly
into the chrome binary. We currently do this for everything other than the theme
images and localized strings. Putting the data directly into chrome makes it
easier for the renderer process to access resources because it doesn't have to
go to disk. To work around this, on linux we would have to load the data
resource in the renderer process before sandboxing it. This is similar to what
we already do for localized resources in the renderer process.
This has the benefit that we don't need to maintain different resource file
definitions across platforms, it would be sufficient to update the main .rc
file. We also don't have to change any of the callers or the current use of
resource IDs throughout the code base. We just need to change ResourceBundle to
handle the new data file.
This would also work cross platform, so we may eventually be able to move
Windows over to it so we all use the same system.
Data File Format version 4
uint32 file version
uint32 number of resources in the file
uint8 encoding of the text resources in the file
&lt;uint16 resource id, uint32 file offset&gt;\*
two zero bytes, uint32 end of last file
&lt;resource data&gt;\*
Unsigned ints will be little endian byte order and strings will be encoded in
conformity to the encoding field. Valid encodings correspond to the following
values:
0 -&gt; Binary: PAK file is not expected to contain any text resources.
1 -&gt; UTF8: PAK file may contain binary data, but all text resources are
encoded in UTF8.
2 -&gt; UTF16: PAK file may contain binary data, but all text resources are
encoded in UTF16.
The length of each resource can be computed by subtracting from the start of the
next resource. To aid in computing the last resource, we include an extra index
entry with the resource id set to zero.
**Other Ideas**
Leave everything in code.
This would work for data blobs, but it doesn't match the current method of
having a separate dll per locale. We could possibly put all the locale strings
into chrome.dll, but this would require more work for indexing the values
properly (i.e., GRIT would no longer work). It would also make it harder to
transition to a single language download.
Leave everything in code, but in .so files (dlls)
Each data resource would become a .c file which we compile into a single .so.
This is very similar to the current approach on Windows and would allow us to
embed data resources into the main chrome.so. This might also be a bit faster to
load because it doesn't have to parse the data file index. The only drawback to
this approach is that it loads data as code. On Windows, you can load a dll with
LoadLibraryEx(LOAD_AS_DATAFILE) which prevents code execution in the case of a
corrupt file. I don't know of a way to do something similar on Linux.
