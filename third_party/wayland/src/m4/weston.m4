dnl
dnl Copyright © 2016 Quentin “Sardem FF7” Glidic
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a
dnl copy of this software and associated documentation files (the "Software"),
dnl to deal in the Software without restriction, including without limitation
dnl the rights to use, copy, modify, merge, publish, distribute, sublicense,
dnl and/or sell copies of the Software, and to permit persons to whom the
dnl Software is furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice (including the next
dnl paragraph) shall be included in all copies or substantial portions of the
dnl Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
dnl THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
dnl FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
dnl DEALINGS IN THE SOFTWARE.
dnl

dnl WESTON_SEARCH_LIBS(PREFIX, search-libs, function, [action-if-found], [action-if-not-found], [other-libraries])
dnl WESTON_SEARCH_LIBS is a wrapper around AC_SEARCH_LIBS with a little difference:
dnl action-if-found is called even if no library is required
AC_DEFUN([WESTON_SEARCH_LIBS], [
	weston_save_LIBS=${LIBS}
	AC_SEARCH_LIBS([$3], [$2], [$4], [$5], [$6])
	AS_CASE([${ac_cv_search_][$3][}],
		['none required'], [$4],
		[no], [],
		[$1][_LIBS=${ac_cv_search_][$3][}]
	)
	AC_SUBST([$1][_LIBS])
	LIBS=${weston_save_LIBS}
])
