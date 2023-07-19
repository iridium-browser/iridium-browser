/*
 * Copyright © 2015 Samsung Electronics Co., Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

/*
 * Common main() for test programs.
 */

#include <stdio.h>
#include <stdlib.h>

#include "zunitc/zunitc.h"

int
main(int argc, char* argv[])
{
	bool helped = false;
	int rc = zuc_initialize(&argc, argv, &helped);

	if ((rc == EXIT_SUCCESS) && !helped) {
		/* Stop if any unrecognized parameters were encountered. */
		if (argc > 1) {
			printf("%s: unrecognized option '%s'\n",
			       argv[0], argv[1]);
			printf("Try '%s --help' for more information.\n",
			       argv[0]);
			rc = EXIT_FAILURE;
		} else {
			rc = ZUC_RUN_TESTS();
		}
	}

	zuc_cleanup();
	return rc;
}
