---
breadcrumbs:
- - /developers
  - For Developers
page_name: windows-binary-sizes
title: Windows Binary Sizes
---

Chrome binaries sometimes increase in size for unclear reasons and investigating
these regressions can be quite tricky. There are a a few tools that help with
investigating these regressions or just looking for wasted bytes in DLLs or
EXEs. The tools are:

*   New in 2021:
            [SizeBench](https://devblogs.microsoft.com/performance-diagnostics/sizebench-a-new-tool-for-analyzing-windows-binary-size/)
            is a Microsoft tool that does some sophisticated analysis of binary
            size. [More information can be found
            here.](https://devblogs.microsoft.com/performance-diagnostics/sizebench-a-new-tool-for-analyzing-windows-binary-size/)
            If you can't download it from the store then you can clone
            <https://github.com/MattiasC85/Scripts> and then use "powershell
            OSD\\Download-AppxFromStore.ps1 -storeurl
            https://www.microsoft.com/en-us/p/sizebench/9ndf4n1wg7d6" to
            download the files, and then run the SizeBench .appx file to install
            SizeBench. Note that SizeBench is incompatible with lld-link so you
            have to build chrome with "use_lld = false". SizeBench is open
            source and [the repo is here](https://github.com/microsoft/SizeBench).
*   [tools\\win\\ShowGlobals](https://source.chromium.org/chromium/chromium/src/+/main:tools/win/ShowGlobals/)
            - this executable (must be built from a VS solution) uses DIA2 (and
            is based off of the dia2dump sample whose source comes with Visual
            Studio) to print all ‘interesting’ global variables. Just pass the
            path to a PDB file and details about any large or redundant global
            variables will be printed. Large global variables are simple enough
            to understand. Redundant or repeated global variables are when the
            same global ends up in the binary multiple times - sometimes dozens
            or hundreds of times. This happens when a non-integral global
            variable (typically a struct, array, float, or double, or an
            integral type with a non-trivial constructor) is defined as static
            or const in a header file, causing it to be instantiated in multiple
            translation units. The constants kWastageThreshold and
            kBigSizeThreshold can be used to configure how much information
            ShowGlobals reports. Making these command-line parameters is left as
            an exercise for the reader.
*   [tools\\win\\pdb_compare_globals.py](https://source.chromium.org/chromium/chromium/src/+/main:tools/win/pdb_compare_globals.py)
            - this uses ShowGlobals.exe to get the interesting global variables
            from two PE files and then print what has changed, either different
            sizes or different interesting globals. This is designed for
            understanding size regressions, since binary size regressions, even
            when they are mostly due to code size, often have a correlated
            effect on global variables
*   [tools\\win\\pe_summarize.py](https://source.chromium.org/chromium/chromium/src/+/main:tools/win/pe_summarize.py)
            - this displays a summary of the size of the sections in the DLLs or
            EXEs whose paths are passed to it. If two copies of the same DLL/EXE
            are passed to it then the size differences are printed as a summary,
            to make regressions and improvements easier to see. This is most
            often used for measuring progress or seeing what section has grown.
*   [tools\\win\\linker_verbose_tracking.py](https://source.chromium.org/chromium/chromium/src/+/main:tools/win/linker_verbose_tracking.py)
            - this scans the output of "link /verbose" in order to see why a
            particular object file was linked in. This can either be because it
            was listed on the command line (perhaps it was in a source set) or
            because it was referenced (often indirectly) by an object file that
            was listed on the command line.
*   SymbolSort - this is a third party tool [available on
            github](https://github.com/adrianstone55/SymbolSort/) that generates
            various reports, including a sorted-by-size list of all symbols,
            including functions. The -diff option to SymbolSort is particularly
            useful for identifying where a change in size is coming from,
            grouped in various ways. Usage: SymbolSort -in new\\chrome.dll.pdb
            -diff old\\chrome.dll.pdb -out change.txt
*   SymbolSort data can be visualized using webtreemap - see
            <https://github.com/rongjiecomputer/bloat-win> for an example
*   Static initializers waste code size and create process-private data.
            The source to a static_initializers tool that will look for symbol
            names associated with static initializers is in the Chromium repo in
            tools\\win\\static_initializers.

Details of how and when to use these tools are shown below.

If looking for size saving opportunities then “ShowGlobals.exe file.pdb” can be
used to find duplicated or large global variables that may be unnecessary.
Typical (shortened for this document) results look like this - the first set of
entries are duplicated globals, the second set of entries are large globals:

```
#Dups  DupSize  Size   Section  Symbol-name
 805      805                    std::piecewise_construct
 3        204                    rgb_red
 3        204                    rgb_green
 3        204                    rgb_blue
 187      187                    WTF::in_place
 4        160                    extensions::api::g_factory
 ...
                 122784    2     kBrotliDictionary
                 65536     2     jpeg_nbits_table
                 57080     2     propsVectorsTrie_index
                 53064     3     unigram_table
                 50364     2     kNetworkingPrivate
                 47152     3     device::UsbIds::vendors_
 ...
```

The actual output is tab separated and can be most easily visualized by pasting
into a spreadsheet to ensure that the columns line up.

Some fixed issues include:

*   webrtc regression was analyzed to find a fix - see[
            https://bugs.chromium.org/p/chromium/issues/detail?id=734631#c14](https://bugs.chromium.org/p/chromium/issues/detail?id=734631#c14)
            for the many steps, and
            [crrev.com/c/567030](https://chromium-review.googlesource.com/c/567030/)
*   We have 216 functions instances of color_xform_RGBA&lt;T&gt;
            totaling 68,184 bytes, down from 300 instances totaling 146496 bytes
            (crbug.com/677450 and crbug.com/680973). This waste was found by
            using SymbolSort's -diff option to investigate a sizes regression.
*   A regression in chrome_watcher.dll was methodically analyzed to
            understand precisely where the size was coming from - see the steps
            listed out starting at [this bug
            comment](https://bugs.chromium.org/p/chromium/issues/detail?id=717103#c24).

The most egregious problems shown by these reports have been fixed but some
remaining issues include:

*   Four copies of 68 byte rgb_red, rgb_green, rgb_blue, rgb_pixelsize
            arrays in chrome.dll and chrome_child.dll -
            https://github.com/libjpeg-turbo/libjpeg-turbo/issues/114
            (unfixable)
*   ~800 copies of std::piecewise_construct -
            <https://connect.microsoft.com/VisualStudio/feedback/details/1059462>
            (next version of VC++?)

The large kBrotliDictionary and jpeg_nbits_table arrays can be seen, but those
are used and are in the read-only section, so there is nothing to be done.

When investigating a regression pdb_compare_globals.py can be used to find out
what large or duplicated global variables have showed up between two builds.
Just pass both PDBs and a summary of the changes will be printed. This uses
ShowGlobals.exe to generate the list of interesting global variables and then
prints a diff.

When investigating a regression (or testing a fix) it can be useful to use
pe_summarize.py to print the size of all of the sections with a PE, or to
compare to PE files (two versions of chrome.dll, for instance). This is the
ultimate measure of success for a change - has it made the binary smaller or
larger, and if so where?

If an unwanted global variable is being linked in then
linker_verbose_tracking.py can be used to help answer the question of “why?”
First you need to find out what object file defines the variable. For instance,
ff_cos_131072 and the other ff_\* globals are defined in rdft.c. When rdft.obj
is pulled in then Chrome gets significantly bigger. Some of this is discussed in
comments #25 to #27 here:
<https://bugs.chromium.org/p/chromium/issues/detail?id=624274#c27.>

In order to get verbose linker output you need to modify the appropriate
BUILD.gn file to add the /verbose linker flag. For chrome.dll I make the
following modification:

```
diff --git a/chrome/BUILD.gn b/chrome/BUILD.gn
index 58586fc..c15d463 100644
--- a/chrome/BUILD.gn
+++ b/chrome/BUILD.gn
@@ -354,6 +354,7 @@ if (is_win) {
"/DELAYLOAD:winspool.drv",
"/DELAYLOAD:ws2_32.dll",
"/DELAYLOAD:wsock32.dll",
+ "/verbose",
\]
if (!is_component_build) {
```

Then build chrome.dll, redirecting the verbose output to a text file:

```
> ninja -C out\release chrome.dll >verbose.txt
```

Alternately you can use the techniques discussed in the [The Chromium Chronicle:
Preprocessing
Source](https://developers.google.com/web/updates/2019/10/chromium-chronicle-7)
to get the linker command and then manually re-run that command with /verbose
appended, redirecting to a text file.

Then linker_verbose_tracking.py is used to find why a particular object file is
being pulled in, in this case mime_util.obj:

```
> python linker_verbose_tracking.py verbose.txt mime_util.obj
```

Because there are multiple object files called mime_util.obj the script will be
searching for for all of them, as shown in the first line of output:

```
> python linker_verbose_tracking.py verbose.txt mime_util.obj
 Searching for \[u'net.lib(mime_util.obj)', u'base.lib(mime_util.obj)'\]
```

You can specify which version you want to search for by including the .lib name
in your command-line search parameter, which is just used for sub-string
matching:

```
> python linker_verbose_tracking.py verbose.txt base.lib(mime_util.obj)
```

Typical output looks like this:

```
> python tools\win\linker_verbose_tracking.py verbose08.txt drop_data.obj
 Database loaded - 3844 xrefs found
 Searching for common_sources.lib(drop_data.obj)
 common_sources.lib(drop_data.obj).obj pulled in for symbol Metadata::Metadata...
     common.lib(content_message_generator.obj)

 common.lib(content_message_generator.obj).obj pulled in for symbol ...
     Command-line obj file: url_loader.mojom.obj
```

In this case this tells us that drop_data.obj is being pulled in indirectly
through a chain of references that starts with url_loader.mojom.obj.
url_loader.mojom.obj is in a source_set which means that it is on the
command-line. I then change the source_set to a static_library and redo the
steps. If all goes well then the unwanted .obj file will eventually stop being
pulled in and I can use pe_summarize.py to measure the savings, and ShowGlobals
to look for the next large global variable.

Sometimes this technique - of changing source_set to static_library - doesn’t
help. In those case it can be important to figure out follow the chain of
symbols and see if it can be broken. In one case
([crrev.com/2559063002](https://codereview.chromium.org/2559063002))
SkGeometry.obj (skia) was pulling in PeriodicWave.obj (Blink) because of log2f.
Investigation showed that SkGeometry.obj referenced the math.h function log2f,
while PeriodicWave.obj defined it as an inline function. The chain was broken by
not defining log2f as an inline function (letting math.h do that) and a
significant amount of code size was saved.

Note that some size regressions only happen with certain build configurations.
Ideally all testing would be done with PGO builds, but that is unwieldy, so
release (non-component?) builds are probably the best starting point. If a size
regression reproes on this configuration then investigate there and fix it. But,
it is advisable to verify the fix on a full official build - with both
is_official_build and full_wpo_on_official set to true. Failing to do this test
can lead to a fix that doesn’t actually help on the builds that we ship to
customers, and this can go unnoticed for months.

See crrev.com/2556603002 for an example of using this technique. In this case it
was sufficient to change a single source_set to a static_library. The size
savings of 900 KB was verified using:

```
> python pe_summarize.py out\release\chrome.dll
Size of out\release\chrome.dll is 42.127872 MB
      name:   mem size  ,  disk size
     .text: 33.900375 MB
    .rdata:  6.325718 MB
     .data:  0.718696 MB,  0.274944 MB
      .tls:  0.000025 MB
  CPADinfo:  0.000036 MB
   .rodata:  0.003216 MB
  .crthunk:  0.000064 MB
    .gfids:  0.001052 MB
    _RDATA:  0.000288 MB
     .rsrc:  0.175088 MB
    .reloc:  1.443124 MB

Size of size_reduction\chrome.dll is 41.211392 MB
      name:   mem size  ,  disk size
     .text: 33.188599 MB
    .rdata:  6.164966 MB
     .data:  0.707848 MB,  0.264704 MB
      .tls:  0.000025 MB
  CPADinfo:  0.000036 MB
   .rodata:  0.003216 MB
  .crthunk:  0.000064 MB
    .gfids:  0.001052 MB
    _RDATA:  0.000288 MB
     .rsrc:  0.175088 MB
    .reloc:  1.409388 MB

Change from out\release\chrome.dll to size_reduction\chrome.dll
     .text: -711776 bytes change
    .rdata: -160752 bytes change
     .data: -10848 bytes change
    .reloc: -33736 bytes change
Total change: -917112 bytes
```
