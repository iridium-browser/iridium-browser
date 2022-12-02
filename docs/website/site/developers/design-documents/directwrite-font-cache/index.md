---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: directwrite-font-cache
title: DirectWrite Font Cache (obsolete)
---

# The font cache is obsolete. For the current implementation, see the [DirectWrite font proxy](/developers/design-documents/directwrite-font-proxy)

# Abstract:

With Chrome version 38 DirectWrite has been enabled by default on Windows
Chrome. With Chrome’s secure sandbox architecture DirectWrite poses a basic but
significant problem. DirectWrite relies on Font Cache Service for system
installed fonts. It communicates with Font Cache service over Windows mostly
undocumented and traditional IPC mechanism of ALPC (Asynchronous Local Procedure
Call.). Due to severe sandbox restrictions DirectWrite is not able to
communicate to Font Cache service from Chrome Renderer process. This causes main
hurdle for implementation and usage of DirectWrite from Renderer process.
DirectWrite allows loading of Custom Fonts separately from System Fonts. We used
this concept to enable DirectWrite in current release by loading system font
files as custom fonts. Though this mechanism currently works, it definitely adds
to load time for renderer (Latency range of 40th percentile less than ~226ms and
90th percentile ~3000ms). To solve this load latency issue we are implementing
Font Cache in the Browser process, which will be shared by all renderers. The
Font Cache will be scoped only for loading all font families and such properties
during initial DirectWrite font enumeration process. Individual font file for
Glyphs will still be directly loaded from disk.

## .

## Implementation:

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/sr3ilNMP6rJv-ADbjZC1BFQ/image?w=624&h=263&rev=1&ac=1"
height=263px; width=624px;>

Inside content/renderer most of the code which deals with all DirectWrite custom
font collection details is in file
[renderer_font_platform_win.cc/.h](https://code.google.com/p/chromium/codesearch#chromium/src/content/renderer/renderer_font_platform_win.cc).
The code is comprised mainly of the following classes that DirectWrite requires
for loading custom font collection.

FontCollectionLoader: (implementing IDWriteFontCollectionLoader)

Function of interest in this class is CreateEnumeratorFromKey, which returns a
font file enumerator to the caller.

FontFileEnumerator: (implementing IDWriteFontFileLoader)

This class provides one way enumerator using two function MoveNext and
GetCurrentFile. MoveNext lets you move cursor to next file in the list where as
GetCurrentFile gets you file at the cursor. We create a reference to font file
at cursor using our custom font file loader and return that reference to caller.

FontFileLoader: (implementing IDWriteFontFileLoader)

As name suggests this class is responsible for loading actual font file whether
it is from disk, resources or somewhere else. This class returns a new instance
of FontFileStream for the file.

FontFileStream: (implementing IDWriteFontFileStream)

This class actually lets call read bytes and fragments of Font File. DirectWrite
during initial phase of building collection seem to use methods provided by this
class to read various data table entries, which include information such as Font
Families contained in this file. Main function implemented here is
ReadFileFragment(), which returns binary data starting a specified offset and of
specified size.

The journey for all this class maze starts from a call to function
IDWriteFactory::CreateCustomFontCollection(). We supply our own custom
FontCollectionLoader as parameter to this function so that DirectWrite ends up
calling us for loading of the actual collection.

\[ Before calling CreateCustomFontCollection we load list of system installed
fonts from registry (HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows
NT\\CurrentVersion\\Fonts.). This list is in the form of key value pair, where
usually key is the name of the main font family contained in the file e.g. Arial
(TrueType) and value is the path to file which contains that family. e.g.
arial.ttf. When full path is not specified system uses standard font location
such as \\windows\\fonts. In our current implementation we ignore all font files
which have absolute path specified. We are doing this in order to avoid any
clashes with our sandbox policy. e.g. if path for a file is c:\\program
files\\xyz\\arial.ttf, we cannot load this file from renderer as this particular
path is sandboxed. We have added Windows standard font location such as
\\windows\\fonts into our sandbox policy to load font files from there. \]

DirectWrite then invokes function to create an enumerator to load all font files
through our FontCollectionLoader. We expose instance to our custom enumerator
for loading these files (FontFileEnumerator). Then DirectWrite enumerates and
creates instance of each font file using MoveNext and GetCurrentFile functions.
Seemingly DirectWrite goes through entire font file collection for generating a
copy of local cache in some undocumented data structure. Through
FontFileEnumerator and FontFileLoader when we return instance to FontFileStream
to represent single font file in our collection, DirectWrite calls
ReadFileFragment multiple times to get required data to fill internal data
structures. After entire enumeration is done, DirectWrite then will satisfy some
of the queries such as how many font families are available etc, from internal
data structures for actual Glyphs it again creates instance of our
FontFileStream using the key that we provided during enumeration time.

Given above flow we can clearly see that DirectWrite is dealing with Font
Collection in two phases, one phase is to load and enumerate entire collection
to read all font files for properties and store them in internal data structure,
the other phase is to actually read glyphs and data from specific font file. Our
theory is that the font file collection enumeration is the key part contributing
to initial latency and we are trying to minimize that using our font cache. One
problem with loading all font files that are installed on the system is that
they are large in numbers. From UMA numbers 55th percentile files to be loaded
are ~340. Which means every new renderer instance will go through loading of
those many files. Given our understanding of how antivirus and antimalware
programs work, we think that these programs sometime add checks for each file
load operation, which adds to the latency.

As mentioned above we want to provide cache implementation only for phase 1 Font
File enumeration of DirectWrite. In our experiments we observed that when
calling ReadFileFragment for the same file such as arial.ttf in various runs,
DirectWrite actually asks for same offset and chunk sizes. If we cache these
offsets and chunk sizes into single file during this enumeration phase then we
may avoid loading of all these font files at least during the startup. In second
phase we are little less worried as the it is very unlikely that any webpage or
application in renderer ends up loading all font files simultaneously. In our
experiments we saw maximum of upto 3-4 font files loaded in phase 2.

For Cache implementation we plan to go through DirectWrite font file enumeration
noting down all offsets and chunk sizes along with actual chunk bytes in Cache
File. We run this process either through Chrome Browser or Chrome utility
process. In our experiment we found that for some files like simunb.ttf,
DirectWrite ends up asking for entire file after first few chunks. To avoid
caching whole file simsunb.ttf (~16MB.) We will check on percentage of total
file to be cached against preset limit such as 70% ([Histogram of total fonts
with respective cache sizes](#histogram)) and only allow caching chunks if
criteria satisfies. During actual enumeration inside renderer we will always
supply alternative path (That is of direct loading from disk) for any font file
that is asked by DirectWrite during enumeration which is not present in our
cache.

We will store this cache file under user profile folder. We will have to provide
mechanism to check for consistency of the file using either checksum or write
completed mark. Once cache file is created browser process will open this file
during initialization and map into a shared memory section accessible in read
only form to all renderers. Renderer will first try to open the section for font
cache during enumeration process. If unable to find the section renderers will
fall back to old font file loading path.

Caching merged segments:

As briefly mentioned above, through ReadFileFragment function of FontFileStream,
DirectWrite reads various data segments inside font file. In our experiments
these segments overlap each other. Sometimes DirectWrite keeps querying several
subset of a segment which it read in some previous call. In our caching strategy
we wait for all these segments to be read and keep track of these offsets and
length pairs as DirectWrite reads font file. For an example look at sample log
of following segments that it read for arial.ttf font file in our trials.

<table>
<tr>

<td>No.</td>

<td>Starting Offset</td>

<td>Data Chunk Length</td>

</tr>
<tr>

<td>1</td>

<td>0</td>

<td>117</td>

</tr>
<tr>

<td>2</td>

<td>0</td>

<td>7</td>

</tr>
<tr>

<td>3</td>

<td>0</td>

<td>16</td>

</tr>
<tr>

<td>4</td>

<td>0</td>

<td>12</td>

</tr>
<tr>

<td>5</td>

<td>12</td>

<td>368</td>

</tr>
<tr>

<td>6</td>

<td>504</td>

<td>96</td>

</tr>
<tr>

<td>7</td>

<td>380</td>

<td>54</td>

</tr>
</table>

Figure 1: Shows how segments overlap in actual reading of Arial.ttf font file.

During closing of this font file, we fold/merge all these chunks of segments so
to create unified caching segments. As could be seen in Figure 1, there are
several segments that start from offset 0 and are of various sizes, also there
are some segments which extended already encountered segment because of overlap.
In Figure 1, segments numbers 2 to 4 are all complete subsets of segments number
1. In this particular case we fold all these subsets into single to be cached
segment of (offset: 0, length: 117). Segment number 5 is an overlapping segment
which starts within (0, 117) but extends beyond it. In this case we extend our
to be cached segment of (0, 117) to be (0, 368). If you next look at segment
number 7, then we also need to combine this segment into our first to be cached
segment of (0, 368) as it is immediate next segment. In conclusion we cache our
first segment as (0, 442) which is representative of segment numbers 1 to 5 and
7. We will store segment number 6 separately in our cache. With this logic our
folded/merged cached segments become:

<table>
<tr>

<td>No.</td>

<td>Starting Offset</td>

<td>Data Chunk Length</td>

</tr>
<tr>

<td>1</td>

<td>0</td>

<td>442</td>

</tr>
<tr>

<td>2</td>

<td>504</td>

<td>96</td>

</tr>
</table>

Figure 2: Shows how the segments get folded/merged while caching.

During read phase we check if requested segment lies within boundaries of cached
segments and provide data for request from cache.

## Font Cache File Format:

CacheFileHeader:

Declaration:

struct CacheFileHeader {

UINT32 file_signature;

UINT32 magic_completion_signature;

UINT32 version;

UINT32 undefined\[5\];

};

Details:

file_signature: Contains values identifying this file type. Current value is
0x4D4F5243, which denote ascii letters ‘CROM’.

magic_completion_signature: Contains specific value which indicate that file has
been fully written. This is to minimally ensure consistency of the file. Current
value is 0x454E4F44, which denote ascii letter ‘DONE’

version: Contains version of file format.

undefined: Some extra space to accommodate for future addition of fields into
this file.

CacheFileEntry:

Declaration:

struct CacheFileEntry {

UINT64 file_size;

DWORD entry_count;

wchar_t file_name\[kMaxFontFileNameLength\];

};

Details:

CacheFileEntry is entry per font file. e.g. arial.ttf and arialb.ttf will have
two separate entries.

We will have fixed space for font file name. This could be problematic for font
file names which exceed our limit and are a letter or two off at the end. We
will have to ignore such entry from adding to font cache for now.

file_size: File size here refers to original file size for which this font file
entry exists. e.g. size of arial.ttf. This size is required when DirectWrite
calls GetFileSize function of FontFileStream class.

entry_count: It contains total number of CacheFileOffsetEntry records following
this CacheFileEntry record.

file_name: Name of the original .ttf/.ttc file.

CacheFileOffsetEntry:

struct CacheFileOffsetEntry {

UINT64 start_offset;

UINT64 length;

/\* BYTE blob\[\]; // Place holder for the blob that follows. \*/

};

Details:
Array of CacheFileOffsetEntry records follow single CacheFileEntry record. Size
of this array is specified in entry_count_ field of CacheFileEntry.

start_offset: Contains starting offset within original file that was asked by
DirectWrite during enumeration in call to ReadFileFragment.

length: This contains value from fragmet_size parameter of ReadFileFragment,
which as passed by DirectWrite during initial enumeration.

BYTE blob\[\]: This is just a place holder. What follows each
CacheFileOffsetEntry record is actual blob of data that was returned to
DirectWrite during initial enumeration. This blob should be copy of exact
contents of original file such as arial.ttf or times.ttf.

\*\* Our assumption here is that no font file has duplicate record in our cache.

\*\* Another assumption is that if for some reason we find font file cache
invalid, we just recreate it instead of repairing it.

## Font Cache File Layout:

<table>
<tr>

<td>CacheFileHeader</td>

<td> <table></td>
<td> <tr></td>

<td><td>file_signature</td></td>

<td><td>magic_signature</td></td>

<td><td>version</td></td>

<td><td>undefined</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>file_size</td></td>

<td><td>entry_count</td></td>

<td><td>file_name</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileOffsetEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>start</td></td>

<td><td>length</td></td>

<td><td>Actual Cached Data Segment</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileOffsetEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>start</td></td>

<td><td>length</td></td>

<td><td>Actual Cached Data Segment</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>...</td>

</tr>
<tr>

<td>CacheFileEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>file_size</td></td>

<td><td>entry_count</td></td>

<td><td>file_name</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileOffsetEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>start</td></td>

<td><td>length</td></td>

<td><td>Actual Cached Data Segment</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileOffsetEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>start</td></td>

<td><td>length</td></td>

<td><td>Actual Cached Data Segment</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>CacheFileOffsetEntry</td>

<td> <table></td>
<td> <tr></td>

<td><td>start</td></td>

<td><td>length</td></td>

<td><td>Actual Cached Data Segment</td></td>

<td> </tr></td>
<td> </table></td>

</tr>
<tr>

<td>...</td>

</tr>
</table>

## Font In-memory Cache:

Internally cache is represented by a map of font file name to a list of data
segments cached from that file. Here in the list we just store start, length and
pointer inside original cache file, which is memory mapped.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/shy9MpsyCNfdhVX7zqgAefA/image?w=594&h=214&rev=1&ac=1"
height=214px; width=594px;>

## Histogram of total fonts with cached percentile.

Following histogram shows frequency of fonts on a typical Windows 8 system in
reference to their respective cached size. Here we are referring to respective
cache size as total number of bytes that need to be present in cache file
compared to original font file.

<img alt="dwrite-font-caching-histogram.png"
src="https://lh5.googleusercontent.com/pShkIUuF5tXWbeqv8F7fPdPNkE-FBdEK-8QQhLuUmX5wkS-tIlsKOMBflGL83n_ZHr3ji4rPZlwe3xczxk6_U4_duYOYfOHXvCojTGYP0hYXPwiRkOQ3yiGwybij8dRwfg"
height=418px; width=566px;>

## References:

Microsoft DirectWrite API documentation.

<http://msdn.microsoft.com/en-us/library/windows/desktop/dd371554(v=vs.85).aspx>

DirectWrite font system in comparison to GDI.

<http://blogs.msdn.com/b/text/archive/2009/04/15/introducing-the-directwrite-font-system.aspx>
