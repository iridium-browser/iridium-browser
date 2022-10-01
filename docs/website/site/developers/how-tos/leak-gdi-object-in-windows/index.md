---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: leak-gdi-object-in-windows
title: Leak GDI Objects in Windows
---

## Tools

*   Windows Task Manager
    *   Add "GDI Objects" column by \[View\]-\[Select Columns...\]
    *   You may also want to add "Handle", "Thread" and "USER Objects"
*   GDIView (works Win7 32bit and 64bit)
    *   <http://www.nirsoft.net/utils/gdi_handles.html>
    *   Note: GdiQueryTable (undocumented GDI32.DLL Win32 API) and
                PEB-&gt;GetSharedGdiHandleTable don't work on Win7.
    *   I'm studying how to enumerate GDI handles on Win7.

## Anti Patterns

### DeleteObject with HICON

**DeleteObject WIN32 API works all GDI objects except for icon**
([97559](http://code.google.com/p/chromium/issues/detail?id=97559))

We must use DestroyIcon.

> In tabe_view.cc(237)

> <http://src.chromium.org/viewvc/chrome/trunk/src/views/controls/table/table_view.cc?annotate=101329>

> 810 HICON empty_icon = 811
> IconUtil::CreateHICONFromSkBitmap(canvas.ExtractBitmap()); 812
> ImageList_AddIcon(image_list, empty_icon); 813 ImageList_AddIcon(image_list,
> empty_icon); 814 DeleteObject(empty_icon);

### Leak Screen DC aka GetDC(NULL)

GetDC(NULL) returns newly created screen DC. So, you must release it by using
ReleaseDC. You may want to use ScopedGetDC
(<http://codesearch.google.com/#OAMlx_jo-ck/src/base/win/scoped_hdc.h>)

**I would like to draw in bitmap** (<http://crbug.com/98523>)

> In print_web_view_helper_win.cc(237)

> <http://src.chromium.org/viewvc/chrome/trunk/src/chrome/renderer/print_web_view_helper_win.cc?annotate=103082>

> 235 // Page used alpha blend, but printer doesn't support it. Rewrite the

> 236 // metafile and flatten out the transparency.

> 237 HDC bitmap_dc = CreateCompatibleDC(GetDC(NULL));

> 238 if (!bitmap_dc)

**Easy way to know screen DPI**:

> In chrome_content_utility_cline.cc(299)

> <http://src.chromium.org/viewvc/chrome/trunk/src/chrome/utility/chrome_content_utility_client.cc?annotate=101911>

> 299 int screen_dpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSX);

> 300 xform.eM11 = xform.eM22 =

> 301 static_cast&lt;float&gt;(screen_dpi) /
> static_cast&lt;float&gt;(render_dpi);

### Play Enhanced Metafile Record More Than Once

Oops, some devices don't support alpha blending. We should do alpha blending by
ourselves by using bitmap DC (<http://crbug.com/98523>)

Since, metafile records having GDI object creation command and stores into
*lpHandleTable* of second parameter of **EnhMetaFileProc**.

> In crhome/renderer/print_web_view_helper_win.cc(40-69)

> <http://src.chromium.org/viewvc/chrome/trunk/src/chrome/renderer/print_web_view_helper_win.cc?annotate=103082>

> 40 // Play this command to the bitmap DC.

> 41 PlayEnhMetaFileRecord(\*bitmap_dc, handle_table, record, num_objects);

> ...

> 68 // Play this command to the metafile DC.

> 69 PlayEnhMetaFileRecord(dc, handle_table, record, num_objects);

## References

*   GdIQueryTable
    *   Explanation of GDI handle table
        *   <http://www.fileinview.com/chms/Windows%20Graphics%20Programming%20Win32%20GDI%20and%20DirectDraw/Win32GDI/32.htm>
    *   Implementation of GDIQueryTable in
                [ReactOS](http://www.reactos.org)
        *   <http://doxygen.reactos.org/d9/db3/dll_2win32_2gdi32_2misc_2misc_8c_a4086f8e85c3c92a3c6d518ef5bf8a690.html>
    *   WinXP/32bit returns 0x00410000.
*   Process Environment Block (PEB)
    *   <http://en.wikipedia.org/wiki/Win32_Thread_Information_Block>
    *   <http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/NT%20Objects/Thread/TEB.html>
    *   reinterpret_cast&lt;PEB\*&gt;(__readfsdword(0x30))-&gt;GdiSharedHandleTable
                (PEB+0x94)
*   GDI Object Handle Encoding
    *   \[0:15\] Index
    *   \[16:22\] Object Type (1=DC, 4=Region, 5=Bitmap, 8=Pallete,
                A=Font, 10=Brush, 21=EnhMF, 30=Pen, 50=ExtPen)
    *   \[23:23\] Stock Object
    *   \[24:31\] Unknown
